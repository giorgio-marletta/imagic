#ifndef IMAGIC_BACKEND_CONNECTORS_H
#define IMAGIC_BACKEND_CONNECTORS_H

#include <cstdint>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <unordered_map>
#include "message.h"

u_int32_t min(uint32_t a, uint32_t b);
void read_bytes(int sockfd, unsigned char *buffer, uint32_t message_length);
void write_bytes(int sockfd, unsigned char *buffer, uint32_t message_length);
void send(int sockfd, const message *msg);
message *receive(int sockfd);

class client_connector {
    private:
        uint32_t current_request_id_;
        unsigned int n_server_;
        std::queue<message *> *message_queue_;
        std::unordered_map<uint32_t, std::vector<int>> *request_map_;
        std::mutex *read_mutex_, *write_mutex_, *write_count_mutex_;
        inline static unsigned int write_count_ = 0;
    public:
        client_connector(unsigned int n_server, std::queue<message *> *message_queue, std::unordered_map<uint32_t, std::vector<int>> *request_map, std::mutex *read_mutex, std::mutex *write_mutex, std::mutex *write_count_mutex);
        void accept_requests();
        void queue_request(int client_sockfd);
};

class server_connector {
    private:
        int server_sockfd_;
        struct sockaddr_in *server_address_;
        unsigned int server_load_;
        std::unordered_map<uint32_t, std::vector<int>> *request_map_;
        std::mutex *send_request_mutex_, *receive_response_mutex_;
        inline static std::mutex write_mutex_ = std::mutex();
    public:
        server_connector();
        server_connector(sockaddr_in *server_address, std::unordered_map<uint32_t, std::vector<int>> *request_map);
        unsigned int get_server_load() const;
        void set_server_load(unsigned int server_load);
        void send_request_and_receive_response(message *client_message);
        void send_response(message *response);
};

#endif //IMAGIC_BACKEND_CONNECTORS_H
