#include <arpa/inet.h>
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include "connectors.h"
#include "constants.h"

uint32_t min(uint32_t a, uint32_t b) {
    return (a<b)?a:b;
}

void read_bytes(int sockfd, unsigned char *buffer, uint32_t message_length) {
    uint32_t offset = 0;
    int bytes_received;
    do {
        bytes_received = read(sockfd, buffer + offset, min(message_length - offset, CHUNK_SIZE));
        switch(bytes_received) {
            case -1:
                throw std::runtime_error("Socket connection error");
            case 0:
                throw std::runtime_error("Socket connection closed");
            default:
                offset += bytes_received;
        }
    } while(offset < message_length);
}

void write_bytes(int sockfd, unsigned char *buffer, uint32_t message_length) {
    uint32_t offset = 0;
    int bytes_sent;
    do {
        bytes_sent = write(sockfd, buffer + offset, message_length);
        switch(bytes_sent) {
            case -1:
                throw std::runtime_error("Socket connection error");
            case 0:
                throw std::runtime_error("Socket connection closed");
            default:
                offset += bytes_sent;
        }
    } while(offset < message_length);
}

//CONNECTOR CLIENT SIDE

client_connector::client_connector(std::queue<message *> *message_queue) : message_queue_(message_queue) {}

void client_connector::accept_requests() {
    //CREATE SOCKET
    struct sockaddr_in server_address, client_address;

    const char *load_balancer_address(std::getenv("LOAD_BALANCER_ADDRESS"));
    int load_balancer_port = std::stoi(std::getenv("LOAD_BALANCER_PORT"));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(load_balancer_address);
    server_address.sin_port = htons(load_balancer_port);
    int server_length = sizeof(server_address);

    //CONNECTION WITH CLIENT
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(server_sockfd, (struct sockaddr *) &server_address, server_length);
    listen(server_sockfd, QUEUE_LENGTH_CONNECTIONS);
    std::cout << *OUTPUT_IDENTIFIER << "Waiting for connections from client..." << std::endl;

    while (true) {
        socklen_t client_length = sizeof(client_address);
        int client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_length); //every connector has a sockfd

        std::thread t(&client_connector::queue_request, this, client_sockfd);
        t.detach();
    }
}

void client_connector::queue_request(int client_sockfd) {
    try {
        std::cout << *OUTPUT_IDENTIFIER << "Connection from client accepted" << std::endl;

        //READ AND PUSH REQUEST
        unsigned char buffer[HEADER_LENGTH];
        read_bytes(client_sockfd, buffer, HEADER_LENGTH);
        auto message_header = new header();
        message_header->deserialize(buffer);
        message_header->set_source_id(client_sockfd);
        uint32_t payload_length = message_header->get_payload_length();
        auto message_payload = new unsigned char[payload_length];
        read_bytes(client_sockfd, message_payload, payload_length);
        auto received_message = new message(message_header, message_payload);
        message_queue_->push(received_message);

        std::cout << *OUTPUT_IDENTIFIER << "NEW MESSAGE RECEIVED AND QUEUED!" << std::endl;
        std::cout << *message_header << std::endl;
    }
    catch (const std::runtime_error& e) {
        close(client_sockfd);
        std::cout << *OUTPUT_IDENTIFIER << e.what() << std::endl;
        return;
    }
}

//CONNECTOR SERVER-SIDE
server_connector::server_connector() {};

server_connector::server_connector(sockaddr_in *server_address) : server_address_(server_address) {
    server_load_ = 0;

    //CONNECTION WITH SERVER
    server_sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    unsigned int len = sizeof(*server_address_);
    if(connect(server_sockfd_, (struct sockaddr *)server_address_, len) == -1) {
        throw std::runtime_error("Socket connection refused");
    }
}

unsigned int server_connector::get_server_load() const {
    return server_load_;
}

void server_connector::set_server_load(unsigned int server_load) {
    server_load_ = server_load;
}

void server_connector::manage_response(message *message) { //Connector server knows the current_message
    auto header = message->get_header();

    //GET THE CLIENT_SOCKFD
    uint32_t client_sockfd = header->get_source_id();
    uint32_t payload_length = header->get_payload_length();

    //SEND REQUEST TO SERVER
    unsigned char buffer[HEADER_LENGTH];
    header->serialize(buffer);
    write_bytes(server_sockfd_, buffer, HEADER_LENGTH);
    write_bytes(server_sockfd_, message->get_payload(), payload_length);

    //GET RESPONSE FROM SERVER
    read_bytes(server_sockfd_, buffer, HEADER_LENGTH);
    auto received_header = message->get_header();
    received_header->deserialize(buffer);
    read_bytes(server_sockfd_, message->get_payload(), received_header->get_payload_length());

    //SEND RESPONSE TO CLIENT
    write_bytes(client_sockfd, buffer, HEADER_LENGTH);
    write_bytes(client_sockfd, message->get_payload(), received_header->get_payload_length());

    //DELETE MESSAGE
    delete message;

    //DECREMENT SERVER LOAD
    server_load_--;
}
