#include <rlib.h>

int connection = 0;
void handle_resize(int pty_fd)
{
    struct winsize ws;
    
  //  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0)
    //{
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0)
        return;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    int original_c = ws.ws_col;
    int original_w = ws.ws_row;
    char * data[1024];
    data[0] = 0;
    sprintf(data,"{resize:%d,%d}",ws.ws_row,ws.ws_col);
    write(connection,data,strlen(data));
    //rterm_clear_screen();
   // ws.ws_col = 10;
  //  ws.ws_row = 10;
  //      ioctl(pty_fd, TIOCSWINSZ, &ws);
      //   ioctl(STDERR_FILENO, TIOCSWINSZ, &ws);
     ws.ws_col = original_c;
    ws.ws_row = original_w;
        ioctl(pty_fd, TIOCSWINSZ, &ws);
      //   ioctl(STDERR_FILENO, TIOCSWINSZ, &ws);
   // }
    
}
void sigwinch_handler(int signo)
{
    if (connection)
        handle_resize(connection); // Update both PTYs
                                         // handle_resize(pty2_fd);
}

size_t read_line(int fd, char *result){
    char buff[1024];
    size_t length = 0;
    result[0] = '\0';
    while(rfd_wait_forever(fd)){
        size_t bytes_read = read(fd,buff,sizeof(buff));
        buff[bytes_read] = 0;
        strcat(result,buff);
       
        if(buff[bytes_read-1] == '\n'){
            break;
        }
    }
    return strlen(result);
    //
    
}

int get_terminal_id(char * list){
    int id = 0;
    while(*list){
        sscanf(list,"%d", &id);
        if(id)
            return id;
            list++;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    connection = net_socket_connect("127.0.0.1", 8888);
   // dup2(connection, STDOUT_FILENO);
   
  //  dup2(STDERR_FILENO, connection);
  //  dup2(STDOUT_FILENO,connection);
   // while(rfd_wait_forever(connection)) {
    //    char buffer[1024];
   //     fread(buffer,1,sizeof(buffer),connection);
    //    write(STDOUT_FILENO,buffer,strlen(buffer));
   // }
    //dup2(connection,STDIN_FILENO);
    
    rrawfd(connection);
    rrawfd(STDIN_FILENO);
    setbuf(stdout,NULL);

struct sigaction sa;
    sa.sa_handler = sigwinch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);


    char buffer[1024];
    write(connection,"list-terminals",strlen("list-terminals"));
    if(rfd_wait_forever(connection)){
        char buffer[1024];
        read(connection,buffer,sizeof(buffer));
        int id = get_terminal_id(buffer);
        if(!id){
            printf("No terminals found\n");
            exit(1);
        }else{
            buffer[0] = 0;
            sprintf(buffer,"subscribe %d",id);

            write(connection,buffer,strlen(buffer));
            handle_resize(connection);
        }
    }
    while (true) {
        if (rfd_wait(connection,100)) {
            size_t bytes_read = read(connection,buffer,sizeof(buffer));//,buffer);
            write(STDOUT_FILENO,buffer,bytes_read);
        } else if(rfd_wait(STDIN_FILENO,10)) {
            size_t  bytes_read = read(STDIN_FILENO,buffer,sizeof(buffer));// ,sizeof(buffer),STDIN_FILENO);
            if(buffer[0] == 24){
                break;
            }
            if(buffer[0] == 18){
                write(connection,"list-terminals",strlen("list-terminals"));
                continue;
            }
            if(buffer[0] == 3){
                char line[1024];
                bytes_read = read_line(STDIN_FILENO,line);
                buffer[0] = 0;
                strcpy(buffer,"subscribe ");
                strcat(buffer,line);
                bytes_read = strlen(buffer);
                rterm_clear_screen();
                printf("Subscribed to %s\n",line);
                write(connection,buffer,bytes_read);
                continue;
            }
            if(buffer[0] < 27){
                
            printf("(%d)\n",buffer[0]);
            
            }
            //buffer[bytes_read-1] = 0;
            
            write(connection,buffer,bytes_read);
        }
    }
    _net_socket_close(connection);
    return 0;
}