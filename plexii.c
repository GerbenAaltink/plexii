#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pty.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <rlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

#define BUF_SIZE 1024 * 1024
#define MAX_TERMINAL_COUNT 20
typedef struct plexii_t {
    bool is_graphic;
    struct pollfd fds[MAX_TERMINAL_COUNT];
    char pty_names[MAX_TERMINAL_COUNT][100];
    int *ptys[MAX_TERMINAL_COUNT];
    unsigned int fd_count;
    unsigned int fd_index;
    char session[L_tmpnam];
    char rc[L_tmpnam];
    // Generate a unique temporary file name
    
} plexii_t;
int active_pty = 0;
void plexii_init(plexii_t * plexii)
{
    plexii->fd_count = 0;
    plexii->fd_index = 0;
    plexii->is_graphic = strstr(getenv("TERM"), "256") || strstr(getenv("TERM"), "xterm") ? true : false;
   // setenv("PS1","\\[\\e]0;${debian_chroot}\\u@\\h: \\w\\a\\]$$",1);
    setenv("PLEXII","1",1);
    setenv("TERM","xterm-256color",1);
    
    printf("<%s>",getenv("PS1"));
}   

//void plexii_set(pollfd fd,char * key, char * value){

//}


// Function to create a PTY pair
int create_pty(char *slave_name)
{
    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1)
    {
        perror("posix_openpt");
        exit(EXIT_FAILURE);
    }

    if (grantpt(master_fd) == -1)
    {
        perror("grantpt");
        exit(EXIT_FAILURE);
    }

    if (unlockpt(master_fd) == -1)
    {
        perror("unlockpt");
        exit(EXIT_FAILURE);
    }

    // Get the name of the slave terminal device
    if (ptsname_r(master_fd, slave_name, 100) != 0)
    {
        perror("ptsname");
        exit(EXIT_FAILURE);
    }

    return master_fd;
}


void write_rc_file(){
    FILE * f = mkstemp("/tmp/plexii.rc");
    write(f->_fileno,"PS1=\"\\[\\e]0;${debian_chroot}\\u@\\h: \\w\\a\\]$$\"\n",strlen("PS1=\"\\[\\e]0;${debian_chroot}\\u@\\h: \\w\\a\\]$$\"\n"));
}

void run_shell_in_pty(int master_fd, const char *slave_name)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process

        // Open the slave PTY
        int slave_fd = open(slave_name, O_RDWR);
        if (slave_fd == -1)
        {
            perror("open slave pty");
            exit(EXIT_FAILURE);
        }

        // Set up the slave PTY as the controlling terminal
        if (setsid() == -1)
        {
            perror("setsid");
            exit(EXIT_FAILURE);
        }

        if (ioctl(slave_fd, TIOCSCTTY, NULL) == -1)
        {
            perror("ioctl TIOCSCTTY");
            exit(EXIT_FAILURE);
        }

        //int server_fd = net_socket_connect("127.0.0.1",8888);
        //dup2(server_fd,slave_fd);

        // Redirect standard input, output, and error to the slave PTY
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        close(slave_fd);

        // Run the shell
       // printf(getenv("PS1"));
     //   exit(0);
        //setenv("PS1","\e[$\{debian_chroot\}\]$\[\033[01;32m[\033[00m$",1);
    //setenv("PLEXII","1",1);
  //  setenv("TERM","xterm-256color",1);
      //  setenv("PS1","${debian_chroot}$",1);
      
        execlp("/bin/bash", "/bin/bash","--norc",NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
}

void enable_raw_mode()
{
    struct termios raw;

    // Get terminal attributes
    tcgetattr(STDIN_FILENO, &raw);

    // Disable canonical mode, echoing, and signals
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);

    // Set terminal attributes
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to restore terminal to normal mode
void disable_raw_mode(struct termios *original)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, original);
}

void resize(int pty_fd,int rows, int cols){
    struct winsize ws;
    ws.ws_col = cols;
    ws.ws_row = rows;
     ioctl(pty_fd, TIOCSWINSZ, &ws);
}
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
    if (active_pty)
        handle_resize(active_pty); // Update both PTYs
                                         // handle_resize(pty2_fd);
}
/*
void write_rc_file(plexii_t * plexii){
    FILE * f = fopen(plexii->rc,"w");
    write(f->_fileno, "PS1=\"\\[\\e]0;${debian_chroot}\\u@\\h: \\w\\a\\]$$\"\n",strlen("PS1=\"\\[\\e]0;${debian_chroot}\\u@\\h: \\w\\a\\]$$\"\n"));

    write(f->_fileno, "HISTFILE=~/.plexii_history\n",strlen("HISTFILE=~/.plexii_history\n"));
}*/
int add_term(plexii_t * plexii)
{
  //  tmpnam(plexii->session);
  //  tmpnam(plexii->rc);
    //write_rc_file(plexii);
    plexii->ptys[plexii->fd_count] = create_pty(plexii->pty_names[plexii->fd_count]);
   
    plexii->fds[plexii->fd_count].fd = plexii->ptys[plexii->fd_count];
  
   
    plexii->fds[plexii->fd_count].events = POLLIN;
    run_shell_in_pty(plexii->ptys[plexii->fd_count], plexii->pty_names[plexii->fd_count]);
    
   
    plexii->fd_index = plexii->fd_count;
    active_pty = plexii->fds[plexii->fd_count].fd;
    // rrawfd(ptys[fd_count]);
    plexii->fd_count++;
    handle_resize(active_pty);
    return plexii->fd_index;
}
int issafe(unsigned char c){
    //32
    if(c >= 1 && c <= 126){
        return 1;
    }else if (c == 10 || c == 13 || c == 9 || c == 11 || c == 12 || c == 8){
        return 1;
    }else{
        return 0;
    }
}

void force_color_red(int fd){
    char * data =  "\033]10;#FF0000\007";
    write(fd,data,strlen(data));
}
void force_color_blue(int fd){
    char * data = "\033]10;#ADD8E6\007";
    write(fd,data,strlen(data));
}
void force_redraw(int fd) {
    char * data = "\e[2J\e[H"; 
    write(fd, data, strlen(data));  
    
}
void force_reset(int fd){
    char * data = "\033[0m\n";
    write(fd,data,strlen(data));
    write(fd,"\033[2J\n",strlen(data));
    write(fd,"\033[H\n",strlen(data));
}

void multiplex_io(plexii_t * plexii)
{

    char buf[BUF_SIZE];
    int n;
    struct termios original;
    tcgetattr(STDIN_FILENO, &original);
    enable_raw_mode();
    force_color_red(STDOUT_FILENO);

    struct pollfd local_fds[3];
   
    // File descriptors to monitor
    // Input from user
    // rrawfd(STDERR_FILENO);

    add_term(plexii);
    // fds[1].fd = pty1_fd;       // PTY 1
    // fds[1].events = POLLIN;

    // fds[2].fd = pty2_fd;       // PTY 2
    // fds[2].events = POLLIN;
    int shared_connection = net_socket_connect("127.0.0.1", 8888);
    while (1)
    {
        active_pty = plexii->ptys[plexii->fd_index];
        local_fds[0].fd = STDIN_FILENO;
        local_fds[1].fd = active_pty;
        local_fds[0].events = POLLIN;
        local_fds[1].events = POLLIN;
        local_fds[2].events = POLLIN;
        local_fds[2].fd = shared_connection;
        poll(local_fds, 3, -1); // Wait for input

        //handle_resize(active_pty);
        // Input from user terminal
        if (local_fds[0].revents & POLLIN)
        {
            n = read(local_fds[0].fd, buf, BUF_SIZE);
            if (*buf == 24)
            {
                break;
            }
            /// if(*buf == '\e'){
            //   rfd_wait_forever(STDIN_FILENO);
            // n =read(STDIN_FILENO, buf, BUF_SIZE);
            if (*buf == 23)
            { // 27up
                //rterm_clear_screen();
                plexii->fd_index++;
                if (plexii->fd_index == plexii->fd_count)
                    plexii->fd_index = 0;

                //printf("Switched to terminal %d!!\n", fd_index + 1);
                active_pty = plexii->ptys[plexii->fd_index];
                //force_redraw(pty_names[fd_index]);
                rterm_clear_screen();
        handle_resize(active_pty);
                continue;
            }
            if (*buf == 14)
            { // ctrl+n
                //rterm_clear_screen();

                //printf("Created terminal %d!!\n", fd_count);
                add_term(plexii);

                rterm_clear_screen();
                handle_resize(active_pty);
                continue;
            }

            //}
            // if(*buf == '\n' && *(buf + 1)==0)
            //   continue;
            // if(strchr(buf,'n')){
            //     exit(0);
            // }
            if (n > 0)
            {
                // write(STDOUT_FILENO,buf,n);
                //  Send input to PTY 1 (for example)
                write(active_pty, buf, n);
            }
        }

         if (local_fds[2].revents & POLLIN)
        {
            n = read(local_fds[2].fd, buf, BUF_SIZE);
            if (*buf == 24)
            {
                break;
            }
            if(n == 0)
            {
                break;
            }
            if(!strncmp(buf,"{resize:",strlen("{resize:"))){
                int cols, rows;
                sscanf(buf + strlen("{resize:"), "%d,%d", &rows, &cols);
                resize(active_pty,rows,cols);
                continue;
            }
            /// if(*buf == '\e'){
            //   rfd_wait_forever(STDIN_FILENO);
            // n =read(STDIN_FILENO, buf, BUF_SIZE);
            if (*buf == 23)
            { // 27up
                //rterm_clear_screen();
                plexii->fd_index++;
                if (plexii->fd_index == plexii->fd_count)
                    plexii->fd_index = 0;

                //printf("Switched to terminal %d!!\n", fd_index + 1);
                active_pty = plexii->ptys[plexii->fd_index];
                //force_redraw(pty_names[fd_index]);
                rterm_clear_screen();
        handle_resize(active_pty);
                continue;
            }
            if (*buf == 14)
            { // ctrl+n
                //rterm_clear_screen();

                //printf("Created terminal %d!!\n", fd_count);
                add_term(plexii);

                rterm_clear_screen();
                handle_resize(active_pty);
                continue;
            }

            //}
            // if(*buf == '\n' && *(buf + 1)==0)
            //   continue;
            // if(strchr(buf,'n')){
            //     exit(0);
            // }
            if (n > 0)
            {
                // write(STDOUT_FILENO,buf,n);
                //  Send input to PTY 1 (for example)
                write(active_pty, buf, n);
            }
        }

        // Output from PTY 1
        if (local_fds[1].revents & POLLIN)
        {
            char new_buf[BUF_SIZE*2];
            int index = 0;
            int new_length = 0;
            n = read(local_fds[1].fd, buf, BUF_SIZE);
            if (n > 0)
            {
                for (int i = 0; i < n; i++)
                {
                    if (issafe(buf[i]))
                    {
                        new_buf[index] = buf[i];
                        index++;
                    }
                    else
                    {
                        if(plexii->is_graphic){
                        new_buf[index] = '\xF0';
                        new_buf[index + 1] = '\x9F';
                        new_buf[index + 2] = '\x98';
                        new_buf[index + 3] = '\x80';
                        index+= 4;
                        }else{
                            new_buf[index] = '%%';
                            index++;
                        }
                    }
                    // printf("%c ", (char)i);
                }
            }
            write(shared_connection,buf,n);
            write(STDOUT_FILENO, new_buf, index); // Write PTY output to terminal
        }
    }
    close(shared_connection);
disable_raw_mode(&original);
force_redraw(STDOUT_FILENO);
rterm_clear_screen();
force_color_blue(STDOUT_FILENO);
printf("Session ended.\n");
}

// int pty1_fd, pty2_fd;

int main()
{
    plexii_t plexii;
    plexii_init(&plexii);
    struct sigaction sa;
    sa.sa_handler = sigwinch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);

    // Multiplex I/O between the two PTYs
    multiplex_io(&plexii);

    return 0;
}
