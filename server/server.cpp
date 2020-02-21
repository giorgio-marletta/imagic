#include <iostream>
#include "constants.h"
#include "server.h"

server::server(unsigned int id) : id_(id) {
    // Set output identifier
    OUTPUT_IDENTIFIER = new std::string("\033[" + std::to_string(id_ % 5 + 32) + "mserver " + std::to_string(id_) + "      |\033[m ");
    // Create load balancer connector
    std::cout << *OUTPUT_IDENTIFIER << "Server " << id_ <<" created" << std::endl;
    lb_connector_ = load_balancer_connector(id);
    std::cout << *OUTPUT_IDENTIFIER << "Load balancer connector " << id << " created" << std::endl;
}

void server::receive_requests() {
    lb_connector_.receive_requests();
}
