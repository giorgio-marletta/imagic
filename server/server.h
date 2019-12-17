#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include "load_balancer_connector.h"

class server {
    private:
        load_balancer_connector lb_connector_;
        unsigned int id_;
    public:
        server(char *address, int port, unsigned int id);
};

#endif //SERVER_SERVER_H