#include <rlib.h>

typedef struct client_t {
    rnet_socket_t * socket;
    unsigned int id;
    bool is_terminal;
    unsigned int subscribed_to;
} client_t ;

client_t * new_client(rnet_socket_t * socket, unsigned int id){
    client_t * client = (client_t *)malloc(sizeof(client_t));
    client->socket = socket;
    client->id = id + 1;
    client->is_terminal = true;
    client->subscribed_to = 0;
    return client;
}

client_t ** clients = NULL;
unsigned int client_count = 0;

char * generate_client_list(){
    char * data = (char *)malloc(4096);
    data[0] = 0;
    strcat(data,"<terminal-list>\n");
    for (int i = 0; i < client_count; i++) {
        if(!clients[i]->socket){
            continue;
        }else if(!clients[i]->is_terminal){
            continue;
        }
        char * client = (char *)malloc(100);
        sprintf(client, "%d", clients[i]->id);
        strcat(data, client);
        strcat(data, "\n");
    }
    strcat(data,"</terminal-list>\n");
    return data;
}

void on_client_connect(rnet_socket_t *client){
    
    clients = (client_t **)realloc(clients, (client_count + 1) * sizeof(client_t *));
    clients[client_count] = new_client(client, client_count);
    client->data = clients[client_count];
    clients[client_count]->socket = client;

    client_count++;
}
void on_client_read(rnet_socket_t *sock){
    client_t * client = (client_t *)sock->data;
    //char *data = (char *)net_socket_read(sock, 4096);
    size_t bytes_length = 0;
    char data[1024];
    bytes_length = read(sock->fd,data,sizeof(data));
    if(bytes_length <= 0){
        net_socket_close(sock);
        return;
    }
    
    if(!strncmp(data,"list-terminals",strlen("list-terminals"))){
        printf("%s requests list.\n", client->socket->name);
        client->is_terminal = false;
        char * list = generate_client_list();
        net_socket_write(client->socket, (unsigned char *)list, strlen(list));
        free(list);
    }else if(!strncmp(data,"subscribe",strlen("subscribe"))){
        printf("%s subscribes.\n", client->socket->name);
        client->is_terminal = false;
        client->subscribed_to = atoi(data + strlen("subscribe") + 1);
        printf("subscribed to %d\n", client->subscribed_to);
    }else if(client->subscribed_to != 0){
        for(unsigned int i = 0; i < client_count; i++){
            if(client->subscribed_to == clients[i]->id){
                net_socket_write(clients[i]->socket, (unsigned char *)data,bytes_length);
            }
        }
    } else{
        for(unsigned int i = 0; i < client_count; i++){
            if(client->id == clients[i]->subscribed_to){
                net_socket_write(clients[i]->socket, (unsigned char *)data, bytes_length);
            }
        }
    }
   // printf("%d(%s): %s\n",client->id,client->is_terminal ? "terminal" : "client",data);
        
}
void on_client_close(rnet_socket_t *sock){
    client_t * client = (client_t *)sock->data;
    printf("%s disconnected.\n", client->socket->name);
    client->socket = NULL;

}


int main(int argc, char *argv[]) {
 rnet_server_t *server = net_socket_serve((unsigned int)atoi(argv[1]), 10);
    server->on_connect = on_client_connect;
    server->on_read = on_client_read;
    server->on_close = on_client_close;
    while (true) {
        if (net_socket_select(server)) {
            printf("Handled all events.\n");
        } else {
            printf("No events to handle.\n");
        }
    }
    return 0;
}