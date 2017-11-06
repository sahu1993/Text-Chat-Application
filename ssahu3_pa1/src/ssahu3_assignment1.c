#include "../include/global.h"
#include "../include/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include <regex.h>

#define DELIM "."
#define MSG_SIZE 350
#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 350
#define BUFFER_SIZE 350

/* function declaration */
void run_as_Server(char *);
void run_as_Client(int);
void in_shell();
void module_Author();
void module_Ip();
void module_Port();
void module_List();
void module_Blocked();
void module_Staistics();
void module_Login();
void module_Refresh();
void module_Send();
void module_Broadcast();
void module_Block();
void module_Unblock();
void module_Logout();
void module_Exit();
void call_command();
void pushNode();
int get_socket_by_ip();
void add_listening_port();
void increse_msg_rec();
void increse_msg_send();
void deleteNode();
void make_client_socket();
void sortedInsert();
void insertionSort();
int sendall();
int recvall();
char * get_ip_by_socket();
void pushClientNode();
void send_list_to_client();
void deleteList();
int is_valid_ip();
int valid_digit();
int is_ip_in_list();
void logout();

int s_port;
int server_flag;
int client_sock;
int c_port;
char *c_port_s;
int g_fdaccpt;

struct Node
{
  int port;
  int socket;
  int msg_send;
  int msg_rec;
  char status[15];
  char hostname[30];
  char ip[16];
  struct Node *next;
};

struct Node* head = NULL;

int main( int argc, char **argv )  {
  /*Init. Logger*/
  cse4589_init_log(argv[2]);

  /*Clear LOGFILE*/
  fclose(fopen(LOGFILE, "w"));

  /*Start Here*/

  if( argc == 3 ){
    if(strcmp(argv[1],"s") == 0){
      s_port = atoi(argv[2]);
      server_flag =1;
      run_as_Server(argv[2]);

    }
    else if(strcmp(argv[1],"c") == 0){
      c_port_s = argv[2];
      c_port = atoi(argv[2]);
      server_flag=0;
      run_as_Client(c_port);
    }
  }
  else if( argc > 3 ){
    cse4589_print_and_log("Too many arguments supplied.\n");
  }
  else {
    cse4589_print_and_log("Two argument expected.\n");
  }
  return 0;
}

void run_as_Server(char *port){

  int  listener_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
	struct sockaddr_in server_addr, client_addr;
	fd_set master_list, watch_list;
  int nbytes;

  int s_port = atoi(port);

	/* Socket */
	listener_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(listener_socket < 0){
		cse4589_print_and_log("Cannot create socket");
  }

	/* Fill up sockaddr_in struct */
	bzero(&server_addr, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(s_port);

    /* Bind */
  int bind_reply = bind(listener_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if(bind_reply < 0){
    cse4589_print_and_log("Bind failed");
  }

    /* Listen */
  int listen_reply = listen(listener_socket, BACKLOG);
  if(listen_reply < 0){
    cse4589_print_and_log("Unable to listen on port");
  }

  FD_ZERO(&master_list);
  FD_ZERO(&watch_list);

  FD_SET(listener_socket, &master_list);
  FD_SET(STDIN, &master_list);

  head_socket = listener_socket;

  while(TRUE){
    memcpy(&watch_list, &master_list, sizeof(master_list));

    /* select() system call. This will BLOCK */
    selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
    if(selret < 0){
      cse4589_print_and_log("select failed.");
    }
    /* Check if we have sockets/STDIN to process */
    if(selret > 0){
      /* Loop through socket descriptors to check which ones are ready */
      for(sock_index=0; sock_index<=head_socket; sock_index+=1){

        if(FD_ISSET(sock_index, &watch_list)){

          /* Check if new command on STDIN */
          if (sock_index == STDIN){

            char command[CMD_SIZE];
            memset(command, '\0', CMD_SIZE);
            if(fgets(command, CMD_SIZE-1, stdin) == NULL)
            exit(-1);

            char *command_w_e = strtok(command, "\n");
            int arg_inc = 0;
            char *arg = strtok(command_w_e, " ");
            char *cmd[3];

            while (arg){
              cmd[arg_inc] = arg ;
              arg_inc += 1;
              arg = strtok(NULL, " ");
            }

            if(strcmp(cmd[0],"AUTHOR")==0){
              module_Author(cmd);
            }else if (strcmp(cmd[0],"IP")==0){
              module_Ip(cmd);
            }else if (strcmp(cmd[0],"PORT")==0){
              module_Port(cmd);
            }else if (strcmp(cmd[0],"LIST")==0){
              module_List(cmd);
            }else if (strcmp(cmd[0],"BLOCKED")==0){
              module_Blocked(cmd);
            }else if (strcmp(cmd[0],"STATISTICS")==0){
              module_Staistics(cmd);
            }else if (strcmp(cmd[0],"exit")==0){
              cse4589_print_and_log("Going out from Shell\n");
            }else{
              cse4589_print_and_log("Unknown Command\n");
            }
          }
          /* Check if new client is requesting connection */
          else if(sock_index == listener_socket){
            caddr_len = sizeof(client_addr);
            fdaccept = accept(listener_socket, (struct sockaddr *)&client_addr, &caddr_len);
            if(fdaccept < 0){
              cse4589_print_and_log("Accept failed.");
            }

            char myIP[16];
            char myHost[30];
            inet_ntop(AF_INET, &(client_addr.sin_addr), myIP, sizeof(myIP));
            unsigned int client_port = ntohs(client_addr.sin_port);
            struct hostent* clienthost;
            clienthost = gethostbyaddr((const char*)&(client_addr.sin_addr),sizeof(client_addr.sin_addr), AF_INET);
            strcpy(myHost,clienthost->h_name);
            pushNode((&head), client_port, fdaccept, myIP, myHost);
            FD_SET(fdaccept, &master_list);
            if(fdaccept > head_socket) head_socket = fdaccept;
          }
          /* Read from existing clients */
          else{
            char *bufferLen = (char*) malloc(sizeof(char)*BUFFER_SIZE);
            memset(bufferLen, '\0', BUFFER_SIZE);

            char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
            memset(buffer, '\0', BUFFER_SIZE);

            int r = recv(sock_index, bufferLen,3,0);
            int msg_len = atoi(bufferLen);

            int s = recvall(sock_index,buffer,msg_len);

            if(r == 0){
              deleteNode(&head,sock_index);
              close(sock_index);
              /* Remove from watched list */
              FD_CLR(sock_index, &master_list);
            }
            else {
              char *p = strtok(buffer, " ");
              int x = 0;
              char *cmd[2];
              while (p != NULL)
              {
                cmd[x] = p;
                x++;
                if(x<=0)
                {
                  p = strtok (NULL, " ");
                }
                else{
                  p = strtok (NULL, "");
                }
              }

              if(strcmp(cmd[0],"lstport")==0){
                add_listening_port(head,cmd[1],sock_index);
                insertionSort(&head);
                struct Node *temp = head;
                send_list_to_client(temp,sock_index,cmd[0]);

              }else if(strcmp(cmd[0],"REFRESH")==0){
                struct Node *temp = head;
                send_list_to_client(temp,sock_index,cmd[0]);

              }else if(strcmp(cmd[0],"LOGOUT")==0) {
                struct Node *temp = head;
                logout(temp,sock_index);
              }else if(strcmp(cmd[0],"BROADCAST")==0){

                char *sender_ip = get_ip_by_socket(head,sock_index);

                int length = strlen(sender_ip)+strlen(cmd[1])+1;
                char lengthbuffer[4];
                sprintf(lengthbuffer,"%-3d", length);

                char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
                strcpy(msg,lengthbuffer);
                strcat(msg,sender_ip);
                strcat(msg," ");
                strcat(msg,cmd[1]);

                int len;
                len = strlen(msg);

                for(int j = 1; j <= head_socket; j++) {
                  if (FD_ISSET(j, &master_list)) {
                    if (j != listener_socket && j != sock_index) {
                      if (sendall(j, msg, &len) == 0) {
                        increse_msg_rec(j,head);
                      }else{
                        //cse4589_print_and_log("Error in Broadcast\n");
                        //cse4589_print_and_log("j %d\n",j );
                      }
                    }
                  }
                }
                increse_msg_send(sock_index,head);
                char rel[10] = "RELAYED";
                char ip_local[17] = "255.255.255.255";
                cse4589_print_and_log("[%s:SUCCESS]\n",rel);
                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", sender_ip,ip_local,cmd[1]);
                cse4589_print_and_log("[%s:END]\n",rel);
                free(msg);
              }else{

                int redirect_socket = get_socket_by_ip(head,cmd[0],sock_index);
                char *sender_ip = get_ip_by_socket(head,sock_index);
                char *reciever_ip = get_ip_by_socket(head,redirect_socket);

                int length = strlen(sender_ip)+strlen(cmd[1])+1;
                char lengthbuffer[4];
                sprintf(lengthbuffer,"%-3d", length);

                char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
                strcpy(msg,lengthbuffer);
                strcat(msg,sender_ip);
                strcat(msg," ");
                strcat(msg,cmd[1]);

                int len;
                len = strlen(msg);

                if(sendall(redirect_socket, msg, &len) == 0){
                  increse_msg_rec(redirect_socket,head);
                  increse_msg_send(sock_index,head);
                }else{
                  //cse4589_print_and_log("Error in send\n");
                }
                char rel[10] = "RELAYED";
                cse4589_print_and_log("[%s:SUCCESS]\n",rel);
                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", sender_ip,reciever_ip,cmd[1]);
                cse4589_print_and_log("[%s:END]\n",rel);
                free(msg);
             }
            }
            free(bufferLen);
            free(buffer);
          }
        }
      }
    }
  }
}

void run_as_Client(int port){

  int c_head_socket, c_selret, c_sock_index ;
  fd_set c_master_list, c_watch_list;

  /* Zero select FD sets */
  FD_ZERO(&c_master_list);
  FD_ZERO(&c_watch_list);
  FD_SET(STDIN, &c_master_list);
  c_head_socket = STDIN;

  while(TRUE){

    memcpy(&c_watch_list, &c_master_list, sizeof(c_master_list));

    c_selret = select(c_head_socket + 1, &c_watch_list, NULL, NULL, NULL);
    if(c_selret < 0){
      cse4589_print_and_log("select failed.");
    }
    if(c_selret > 0){

      for(c_sock_index=0; c_sock_index<=c_head_socket; c_sock_index+=1){

        if(FD_ISSET(c_sock_index, &c_watch_list)){

          /* Check if new command on STDIN */
          if (c_sock_index == STDIN){
            char command[CMD_SIZE];
            char command_p[CMD_SIZE];

            memset(command, '\0', CMD_SIZE);
            fgets(command, CMD_SIZE-1, stdin) ;
            strcpy(command_p,command);
            char *command_w_e = strtok(command, "\n");
            int arg_inc = 0;
            char *arg = strtok(command_w_e, " ");
            char *cmd[3];

            while (arg){
              cmd[arg_inc] = arg ;
              arg_inc += 1;
              arg = strtok(NULL, " ");
            }

            if(strcmp(cmd[0],"AUTHOR")==0){
              module_Author(cmd);
            }else if (strcmp(cmd[0],"IP")==0){
              module_Ip(cmd);
            }else if (strcmp(cmd[0],"PORT")==0){
              module_Port(cmd);
            }else if (strcmp(cmd[0],"LIST")==0){
              module_List(cmd);
            }else if (strcmp(cmd[0],"LOGIN")==0){
              module_Login(cmd);
              FD_SET(client_sock, &c_master_list);
              if(client_sock>c_head_socket){
                c_head_socket = client_sock;
              }
            }else if (strcmp(cmd[0],"REFRESH")==0){
              module_Refresh(cmd);
            }else if (strcmp(cmd[0],"SEND")==0){
              module_Send(command_p);
            }else if (strcmp(cmd[0],"BROADCAST")==0){
              module_Broadcast(command_p);
            }else if (strcmp(cmd[0],"BLOCK")==0){
              module_Block(cmd);
            }else if (strcmp(cmd[0],"UNBLOCK")==0){
              module_Unblock(cmd);
            }else if (strcmp(cmd[0],"LOGOUT")==0){
              module_Logout(cmd);
            }else if (strcmp(cmd[0],"EXIT")==0){
              module_Exit(cmd);
              FD_CLR(client_sock,&c_master_list);
            }else if (strcmp(cmd[0],"exit")==0){
              cse4589_print_and_log("Going out from Shell\n");
            }else{
              cse4589_print_and_log("Unknown Command\n");
            }
          }else {

            char *bufferLen = (char*) malloc(sizeof(char)*BUFFER_SIZE);
            memset(bufferLen, '\0', BUFFER_SIZE);

            char *buffer_rec = (char*) malloc(sizeof(char)*BUFFER_SIZE);
            memset(buffer_rec, '\0', BUFFER_SIZE);

            int r = recv(client_sock, bufferLen,3, 0);
            int msg_len = atoi(bufferLen);

            int s = recvall(client_sock,buffer_rec,msg_len);

            if(msg_len == 0){
              //cse4589_print_and_log("Error in msg Recieve");
            }else {
              char *cmds[2];
              int i = 0;
              char *p = strtok (buffer_rec, " ");

              while (p != NULL)
              {
                cmds[i] = p;
                i++;
                if(i<=0)
                {
                  p = strtok (NULL, " ");
                }
                else{
                  p = strtok (NULL, "");
                }
              }
              if(strcmp(cmds[0],"list")==0)  {

                  char *arg = strtok (cmds[1], " ");
                  char *info[12];
                  int arg_inc =0;

                  while (arg){
                    info[arg_inc] = arg ;
                    arg_inc += 1;
                    arg = strtok(NULL, " ");
                  }
                  if(arg_inc == 3){
                    int port1 = atoi(info[2]);
                    pushClientNode((&head), info[0], info[1], port1);
                  }else if (arg_inc == 6){
                    int port1 = atoi(info[2]);
                    int port2 = atoi(info[5]);
                    pushClientNode((&head), info[0], info[1], port1);
                    pushClientNode((&head), info[3], info[4], port2);
                  }else if(arg_inc == 9){
                    int port1 = atoi(info[2]);
                    int port2 = atoi(info[5]);
                    int port3 = atoi(info[8]);
                    pushClientNode((&head), info[0], info[1], port1);
                    pushClientNode((&head), info[3], info[4], port2);
                    pushClientNode((&head), info[6], info[7], port3);
                  }else{
                    int port1 = atoi(info[2]);
                    int port2 = atoi(info[5]);
                    int port3 = atoi(info[8]);
                    int port4 = atoi(info[11]);
                    pushClientNode((&head), info[0], info[1], port1);
                    pushClientNode((&head), info[3], info[4], port2);
                    pushClientNode((&head), info[6], info[7], port3);
                    pushClientNode((&head), info[9], info[10], port4);
                  }
                }else if(strcmp(cmds[0],"refresh")==0){

                  deleteList(&head);

                  char *arg = strtok (cmds[1], " ");
                  char *info[12];
                  int arg_inc =0;

                  while (arg){
                    info[arg_inc] = arg ;
                    arg_inc += 1;
                    arg = strtok(NULL, " ");
                  }
                  if(arg_inc == 3){
                    int port1 = atoi(info[2]);
                    pushClientNode((&head), info[0], info[1], port1);
                  }else if (arg_inc == 6){
                    int port1 = atoi(info[2]);
                    int port2 = atoi(info[5]);
                    pushClientNode((&head), info[0], info[1], port1);
                    pushClientNode((&head), info[3], info[4], port2);
                  }else if(arg_inc == 9){
                    int port1 = atoi(info[2]);
                    int port2 = atoi(info[5]);
                    int port3 = atoi(info[8]);
                    pushClientNode((&head), info[0], info[1], port1);
                    pushClientNode((&head), info[3], info[4], port2);
                    pushClientNode((&head), info[6], info[7], port3);
                  }else{
                    int port1 = atoi(info[2]);
                    int port2 = atoi(info[5]);
                    int port3 = atoi(info[8]);
                    int port4 = atoi(info[11]);
                    pushClientNode((&head), info[0], info[1], port1);
                    pushClientNode((&head), info[3], info[4], port2);
                    pushClientNode((&head), info[6], info[7], port3);
                    pushClientNode((&head), info[9], info[10], port4);
                  }
                }
                else{
                  char rec[10] = "RECEIVED";
                  cse4589_print_and_log("[%s:SUCCESS]\n",rec);
                  cse4589_print_and_log("msg from:%s\n[msg]:%s\n", cmds[0],cmds[1]);
                  cse4589_print_and_log("[%s:END]\n",rec);
                }
            }
            free(buffer_rec);
            free(bufferLen);
          }
        }
      }
    }
  }
}

/*------------------------------------------------------------------------------------*/

/*Command function starts*/

void module_Author(char *cmd[]){
  char ubit_name[10] = "ssahu3";
  cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n",ubit_name);
  cse4589_print_and_log("[%s:END]\n",cmd[0]);
}

void module_Ip(char *cmd[]){
  int udp_sock;
  udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(udp_sock<0){
    cse4589_print_and_log("[%s:ERROR\n]", cmd[0] );
    return ;
  }
  struct sockaddr_in google_server_address;
  google_server_address.sin_family = AF_INET;
  inet_pton(AF_INET, "8.8.8.8", &(google_server_address.sin_addr));
  google_server_address.sin_port = htons(53);

  if(connect(udp_sock, (struct sockaddr *) &google_server_address, sizeof(google_server_address))==-1){
    cse4589_print_and_log("[%s:ERROR\n]", cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }
  struct sockaddr_in my_addr;
  int sa_len = sizeof(struct sockaddr);
  if(getsockname(udp_sock, (struct sockaddr *) &my_addr, &sa_len)==-1){
    cse4589_print_and_log("[%s:ERROR\n]", cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }
  char myIP[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(my_addr.sin_addr), myIP, INET_ADDRSTRLEN);
  cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  cse4589_print_and_log("IP:%s\n",myIP);
  cse4589_print_and_log("[%s:END]\n",cmd[0]);
}

void module_Port(char *cmd[]){
  if(server_flag){
    cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
    cse4589_print_and_log("PORT:%d\n",s_port);
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
 }else{
   cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
   cse4589_print_and_log("PORT:%d\n",c_port);
   cse4589_print_and_log("[%s:END]\n",cmd[0]);
 }
 return;
}

void module_List(char *cmd[]){

  cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  struct Node *node = head;
  int counter =1;
  while (node != NULL)
  {
    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", counter, node->hostname, node->ip, node->port);
    counter++;
    node = node->next;
  }
  cse4589_print_and_log("[%s:END]\n",cmd[0]);
  return;
}

void module_Staistics(char *cmd[]){
  cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  struct Node *node = head;
  int counter =1;
  while (node != NULL)
  {
    cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", counter, node->hostname, node->msg_send, node->msg_rec,node->status);
    counter++;
    node = node->next;
  }
  cse4589_print_and_log("[%s:END]\n",cmd[0]);
  return;
}

void module_Login(char *cmd[]){

  client_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(client_sock<0){
    cse4589_print_and_log("[%s:ERROR\n]",cmd[0]);
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }

  struct sockaddr_in server_address;
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  int server_port = atoi(cmd[2]);
  inet_pton(AF_INET, cmd[1], &server_address.sin_addr);
  server_address.sin_port = htons(server_port);

  int fdaccept = connect(client_sock, (struct sockaddr *) &server_address, sizeof(server_address));
  if(fdaccept == -1){
    cse4589_print_and_log("[%s:ERROR]\n", cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }

  fcntl(client_sock, F_SETFL, O_NONBLOCK);
  char msg[30];
  char *flag = "lstport";

  int length = strlen(flag)+strlen(c_port_s)+1;
  char lengthbuffer[4];
  sprintf(lengthbuffer,"%-3d", length);

  strcpy(msg,lengthbuffer);
  strcat(msg,flag);
  strcat(msg," ");
  strcat(msg,c_port_s);

  if(send(client_sock, msg, strlen(msg), 0) == strlen(msg))
  {
    cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
  }else{
    cse4589_print_and_log("[%s:ERROR]\n",cmd[0]);
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
  }
  return ;
}

void module_Send(char cmd[]){
  strtok(cmd, "\n");
  int i = 0;
  char *p = strtok (cmd, " ");
  char *cmds[3];
  while (p != NULL)
  {
    cmds[i] = p;
    i++;
    if(i<=1)
    {
      p = strtok (NULL, " ");
    }
    else{
      p = strtok (NULL, "");
    }
  }

  char *ip = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip,cmds[1]);
  int valid_ip = is_valid_ip(ip);
  free(ip);
  if(valid_ip == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmds[0] );
    cse4589_print_and_log("[%s:END]\n",cmds[0]);
    return ;
  }

  char *ip_c = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip_c,cmds[1]);
  int ip_in_list = is_ip_in_list(head,ip_c);
  free(ip_c);
  if(ip_in_list == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmds[0] );
    cse4589_print_and_log("[%s:END]\n",cmds[0]);
    return ;
  }

   char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);

   int length = strlen(cmds[1])+strlen(cmds[2])+1;
   char lengthbuffer[4];
   sprintf(lengthbuffer,"%-3d", length);

   strcpy(msg,lengthbuffer);
   strcat(msg,cmds[1]);
   strcat(msg," ");
   strcat(msg,cmds[2]);

   int len;
   len = strlen(msg);

   if((sendall(client_sock, msg, &len) == 0))
   {
     cse4589_print_and_log("[%s:SUCCESS]\n",cmds[0]);
     cse4589_print_and_log("[%s:END]\n",cmds[0]);
   }else{
     cse4589_print_and_log("[%s:ERROR\n]", cmds[0] );
     cse4589_print_and_log("[%s:END]\n",cmds[0]);
   }
  free(msg);
}

void module_Broadcast(char cmd[]){
  strtok(cmd, "\n");
  int i = 0;
  char *p = strtok (cmd, " ");
  char *cmds[2];
  while (p != NULL)
  {
    cmds[i] = p;
    i++;
    if(i<=0)
    {
      p = strtok (NULL, " ");
    }
    else{
      p = strtok (NULL, "");
    }
  }

   char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);

   int length = strlen(cmds[0])+strlen(cmds[1])+1;
   char lengthbuffer[4];
   sprintf(lengthbuffer,"%-3d", length);

   strcpy(msg,lengthbuffer);
   strcat(msg,cmds[0]);
   strcat(msg," ");
   strcat(msg,cmds[1]);

   int len;
   len = strlen(msg);

  if((sendall(client_sock, msg, &len) == 0))
  {
    cse4589_print_and_log("[%s:SUCCESS]\n",cmds[0]);
    cse4589_print_and_log("[%s:END]\n",cmds[0]);
  }else{
    cse4589_print_and_log("[%s:ERROR\n]", cmds[0] );
    cse4589_print_and_log("[%s:END]\n",cmds[0]);
  }
  free(msg);
}

void module_Refresh(char *cmd[]){

  char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
  char *send = "sendlist";

  int length = strlen(cmd[0])+strlen(send)+1;
  char lengthbuffer[4];
  sprintf(lengthbuffer,"%-3d", length);

  strcpy(msg,lengthbuffer);
  strcat(msg,cmd[0]);
  strcat(msg," ");
  strcat(msg,send);

  int len;
  len = strlen(msg);

  if((sendall(client_sock, msg, &len) == 0))
  {
    cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
  }else{
    cse4589_print_and_log("[%s:ERROR\n]", cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
  }
  free(msg);
}

void module_Block(char *cmd[]){
  char *ip = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip,cmd[1]);
  int valid_ip = is_valid_ip(ip);
  free(ip);
  if(valid_ip == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }

  char *ip_c = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip_c,cmd[1]);
  int ip_in_list = is_ip_in_list(head,ip_c);
  free(ip_c);
  if(ip_in_list == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }

  //cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  //cse4589_print_and_log("[%s:END]\n",cmd[0]);

}

void module_Unblock(char *cmd[]){
  char *ip = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip,cmd[1]);
  int valid_ip = is_valid_ip(ip);
  free(ip);
  if(valid_ip == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }

  char *ip_c = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip_c,cmd[1]);
  int ip_in_list = is_ip_in_list(head,ip_c);
  free(ip_c);
  if(ip_in_list == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmd[0]);
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }
  //cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  //cse4589_print_and_log("[%s:END]\n",cmd[0]);
}

void module_Blocked(char *cmd[]){
  char *ip = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip,cmd[1]);
  int valid_ip = is_valid_ip(ip);
  free(ip);
  if(valid_ip == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }

  char *ip_c = (char*) malloc(sizeof(char)*MSG_SIZE);
  strcpy(ip_c,cmd[1]);
  int ip_in_list = is_ip_in_list(head,ip_c);
  free(ip_c);
  if(ip_in_list == 0) {
    cse4589_print_and_log("[%s:ERROR]\n",cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
    return ;
  }
  //cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  //cse4589_print_and_log("[%s:END]\n",cmd[0]);
}

void module_Logout(char *cmd[]){

  char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
  char *send = "logoutsend";

  int length = strlen(cmd[0])+strlen(send)+1;
  char lengthbuffer[4];
  sprintf(lengthbuffer,"%-3d", length);

  strcpy(msg,lengthbuffer);
  strcat(msg,cmd[0]);
  strcat(msg," ");
  strcat(msg,send);

  int len;
  len = strlen(msg);

  if((sendall(client_sock, msg, &len) == 0))
  {
    cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
  }else{
    cse4589_print_and_log("[%s:ERROR\n]", cmd[0] );
    cse4589_print_and_log("[%s:END]\n",cmd[0]);
  }
  free(msg);
}

void module_Exit(char *cmd[]){
  close(client_sock);
  cse4589_print_and_log("[%s:SUCCESS]\n",cmd[0]);
  cse4589_print_and_log("[%s:END]\n",cmd[0]);
  exit(0);
  cse4589_print_and_log("after exit 0" );
}

/*command function ends*/

/*------------------------------------------------------------------------------------*/

/*Utility function Start*/

void increse_msg_rec(int socket,struct Node* node){
  while (node != NULL)
  {
    if(node->socket==socket)
    {
      node->msg_rec = node->msg_rec+1;
      break;
    }
    node = node->next;
  }
}

void increse_msg_send(int socket,struct Node* node){
  while (node != NULL)
  {
    if(node->socket==socket)
    {
      node->msg_send = node->msg_send+1;
      break;
    }
    node = node->next;
  }
}

int get_socket_by_ip(struct Node* node, char *ip)
{
  int socketfd =0;
  while (node != NULL)
  {
    if(strcmp(node->ip,ip)==0)
    {
      socketfd = node->socket;
      break;
    }
    node = node->next;
  }
  return socketfd;
}

char * get_ip_by_socket(struct Node* node, int socket){
  char *ip;
  while (node != NULL)
  {
    if(node->socket == socket)
    {
      ip = node->ip;
      break;
    }
    node = node->next;
  }
  return ip;
}

void logout(struct Node* node, int socket){
  char *logout_s = "logged-out";
  while (node != NULL)
  {

    if(node->socket==socket)
    {
      strcpy(node->status,logout_s);
      break;
    }
    node = node->next;
  }
  return;
}

void add_listening_port(struct Node* node, char *port,int socket){
  int lport = atoi(port);
  while (node != NULL)
  {

    if(node->socket==socket)
    {
      node->port = lport ;
      break;
    }
    node = node->next;
  }
  return;
}

void send_list_to_client(struct Node* node, int socket, char *flag){

   char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
   if(strcmp(flag,"lstport")==0){
     strcpy(msg,"list");
   }else{
     strcpy(msg,"refresh");
   }

   while(node != NULL){
     strcat(msg," ");
     strcat(msg,node->hostname);
     strcat(msg," ");
     strcat(msg,node->ip);
     strcat(msg," ");
     char str[5];
     sprintf(str,"%d",node->port);
     strcat(msg,str);
     node = node->next;
   }

   char *msg_send = (char*) malloc(sizeof(char)*MSG_SIZE);
   int length = strlen(msg);
   char lengthbuffer[4];
   sprintf(lengthbuffer,"%-3d", length);

   strcpy(msg_send,lengthbuffer);
   strcat(msg_send,msg);

   int len ;
   len = strlen(msg_send);

   if(sendall(socket, msg_send, &len) == 0){
    // printf("List Sent!\n");
   }else{
     printf("Error in List sending");
   }
   free(msg);
 }

int valid_digit(char *ip_str){
	while (*ip_str) {
		if (*ip_str >= '0' && *ip_str <= '9')
			++ip_str;
		else
			return 0;
	}
	return 1;
}

int is_ip_in_list(struct Node* node, char *ip){
  int result =0;
  while (node != NULL)
  {
  if(strcmp(node->ip,ip)==0)
  {
    result = 1;
    break;
  }
  node = node->next;
  }
  return result;
}

int sendall(int s, char *buf, int *len){
  int total = 0;
  int bytesleft = *len;
  int n;
  while(total < *len) {
    n = send(s, buf+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }
  *len = total;
  return n==-1?-1:0;
}

int recvall(int s, char *buf, int len){
  int total =0;
  int bytesleft = len;
  int n;
  while(total < len) {
    n = recv(s, buf+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }
  len = total;
  return n==-1?-1:0;

}

/* part of this code is taken from geeksforgeeks.com */

void deleteList(struct Node** head_ref){
 struct Node* current = *head_ref;
 struct Node* next;

 while (current != NULL)
 {
 	next = current->next;
 	free(current);
 	current = next;
 }
 *head_ref = NULL;
}

void deleteNode(struct Node** head_ref, int socket)
{
  struct Node* temp = *head_ref, *prev;

	if (temp != NULL && temp->socket == socket)
	{
		*head_ref = temp->next;
		free(temp);
		return;
	}
  while (temp != NULL && temp->socket!= socket)
	{
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL) return;

	prev->next = temp->next;

	free(temp);
  return;
}

void pushNode(struct Node** head_ref, int new_port, int new_socket, char new_ip[], char new_host[]){
	struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
	struct Node *last = *head_ref;

	new_node->port = 0;
  new_node->msg_send = 0;
  new_node->msg_rec = 0;
  new_node->socket = new_socket;
  strcpy(new_node->status,"logged-in");
  strcpy(new_node->ip,new_ip);
  strcpy(new_node->hostname,new_host);

	new_node->next = NULL;
	if (*head_ref == NULL){
  	*head_ref = new_node;
  	return;
	}

	while (last->next != NULL)
		last = last->next;

	last->next = new_node;
	return;
}

void pushClientNode(struct Node** head_ref, char new_host[], char new_ip[], int new_port){

	struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
  struct Node *last = *head_ref;

	new_node->port = new_port ;
  new_node->msg_send = 0;
  new_node->msg_rec = 0;
  new_node->socket = 0;
  strcpy(new_node->status,"logged-in");
  strcpy(new_node->ip,new_ip);
  strcpy(new_node->hostname,new_host);
  new_node->next = NULL;

	if (*head_ref == NULL){
	   *head_ref = new_node;
	    return;
	}

	while (last->next != NULL)
		last = last->next;

	last->next = new_node;
	return;
}

void insertionSort(struct Node** head_ref){
	struct Node *sorted = NULL;

	struct Node *current = *head_ref;
	while (current != NULL)
	{
		struct Node *next = current->next;
    sortedInsert(&sorted, current);
    current = next;
	}
  *head_ref = sorted;
}

void sortedInsert(struct Node** head_ref, struct Node* new_node)
{
	struct Node* current;
	if (*head_ref == NULL || (*head_ref)->port >= new_node->port)
	{
		new_node->next = *head_ref;
		*head_ref = new_node;
	}
	else
	{
		current = *head_ref;
		while (current->next!=NULL && current->next->port < new_node->port)
		{
			current = current->next;
		}
		new_node->next = current->next;
		current->next = new_node;
	}
}

int is_valid_ip(char *ip_str)
{
	int i, num, dots = 0;
	char *ptr;

	if (ip_str == NULL)
		return 0;
	ptr = strtok(ip_str, DELIM);

	if (ptr == NULL)
		return 0;

	while (ptr) {
		if (!valid_digit(ptr))
			return 0;

		num = atoi(ptr);

		if (num >= 0 && num <= 255) {
			ptr = strtok(NULL, DELIM);
			if (ptr != NULL)
				++dots;
		} else
			return 0;
	}

	if (dots != 3)
		return 0;
	return 1;
}

/* part of geeksforgeeks.com code ends*/

/*Utility function Ends*/

