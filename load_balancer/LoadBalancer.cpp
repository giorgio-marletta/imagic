#include "LoadBalancer.h"
#include "Constants.h"

void LoadBalancer::readBytes(unsigned char buffer[],int offset, int n_bytes){

    for(int i=offset; i<n_bytes; i++){
        buffer[i]=message_queue.front();
        message_queue.pop();
    }
}

void LoadBalancer::initializeServerAddresses(){
    server_address[0].sin_family = AF_INET;
    server_address[0].sin_addr.s_addr = inet_addr(CONNECTOR_SERVER_ADDRESS);
    server_address[0].sin_port = htons(CONNECTOR_SERVER_FIRST_PORT);
    for(int i=1; i<N_SERVER; i++){
        server_address[i].sin_family = AF_INET;
        server_address[i].sin_addr.s_addr = inet_addr(CONNECTOR_SERVER_ADDRESS);
        server_address[i].sin_port = htons(server_address[i-1].sin_port+ CONNECTOR_SERVER_OFFSET_PORT);
    }
    cout <<"Server addresses initialized..." << endl;
}

LoadBalancer::LoadBalancer() {

    //INITIALIZATION SERVER ADDRESSES
    initializeServerAddresses();

    //CREATE CLIENT CONNECTOR
    client_connector = new ConnectorClient(&message_queue);
    cout <<"Connector-Client created..." << endl;
    arrayThreads[0] = thread(&ConnectorClient::manageRequest, this->client_connector);

    //CREATE SERVER CONNECTORS
    for(int i=0; i<N_SERVER; i++){
        server_connector[i] = new ConnectorServer(&server_address[i]);
        cout << "Connector-Server[" << i << "] created..." << endl;
    }
}

int LoadBalancer::balance(){

    int server_low_load=0, low_load=0;
    for(int i=0; i<N_SERVER; i++) {
        if ((server_connector[i]->getServerLoad()) <= low_load)
            server_low_load = i;
    }
    return server_low_load;
};

void messageCopyOnBufferConnector(unsigned char message[], int offset, int n_bytes, ConnectorServer* cs){

    cs->writeBuffer(message, n_bytes, offset);
}

void LoadBalancer::manageRequest() {

    while(true) {

        if(!message_queue.empty()) {
            //GET MESSAGE FROM QUEUE
            *current_message = message_queue.front();
            message_queue.pop();
            if(!current_message->getHeader()->getMessageType()){ //Message must be sent in broadcast
                for(int i=0; i<N_SERVER; i++){
                    server_connector[i]->setServerLoad(server_connector[i]->getServerLoad()+1);
                    arrayThreads[i+1] = thread(&ConnectorServer::manageResponse, this->server_connector[i], current_message);
                }
            }
            else{ //Message must be sent to one server only
                int chosen_server = balance();
                server_connector[chosen_server]->setServerLoad(server_connector[chosen_server]->getServerLoad()+1);
                arrayThreads[chosen_server+1] = thread(&ConnectorServer::manageResponse, this->server_connector[chosen_server], current_message);
            }
        }
    }
};