#include <iostream>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <list>
#include <time.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "port.h"
#include "tool.h"
#include "log.h"

using namespace std;

Port::Port() {
}

thread Port::record_time_io() {
  // start a server
  struct sockaddr_in server, client;
  int server_socket, client_socket, c;
  char buf[1000];
  ssize_t read;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(RECORD_TIME_MGT_PORT);
  int yes = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  ::bind(server_socket, (struct sockaddr *)&server, sizeof(server));
  listen(server_socket, 3);
  c = sizeof(struct sockaddr_in);
  while ( (client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
    string ip = inet_ntoa(client.sin_addr);

    read = recv(client_socket, buf, sizeof(buf), 0);
    buf[read] = '\0';
    
    FILE *fp;

    log_trace("%s\n", buf);
    if (strncmp(buf, "query", sizeof("query")) == 0) {
      // get record time
      log_trace("Get the record time");
      fp = fopen(RECORD_TIME_FILE, "r");
      read = fread(buf, sizeof(char), sizeof(buf), fp);
      log_trace("Record time: %s", buf);
      write(client_socket, buf, read);
    }
    else {
      // set record time
      log_trace("Set the record time");
      fp = fopen(RECORD_TIME_FILE, "w");
      fwrite(buf, sizeof(char), read, fp);
    }
    fclose(fp);
  }
}

void Port::start() {
  // start the record time management thread
  thread io_thread(&Port::record_time_io, this);
  int ap_recv_port = 8002;
  int socket_desc;
  struct sockaddr_in server;
  char *file_name;
  char file_name_with_path[100];
  char buf[1024];
  char start_with_file_name[1024];
  int read_num;
  FILE *fp;
  bool fail;

  while (true) {
    sleep(5);
    char *l = get_local_ip("wlan0");
    char local_ip[100];
    strcpy(local_ip, l);
    if (strcmp(local_ip, "0.0.0.0") == 0) {
      continue;
    }
    // get gateway ip address
    char *gateway_ip = get_gateway_ip_from_local_ip(local_ip, sizeof(local_ip));

    // send file to ap
    DIR *d;
    struct dirent *dir;
    d = opendir(FILE_PATH);
    if (d) {
      while ((dir = readdir(d)) != NULL) {
        file_name = dir->d_name;
        if (strncmp(file_name, "f_", 2) != 0) {
          continue;
        }

        socket_desc = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_desc == -1)
        {
          break;
        }
        server.sin_addr.s_addr = inet_addr(gateway_ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(ap_recv_port);
        if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
          break;
        }

        strcpy(file_name_with_path, FILE_PATH);
        strcat(file_name_with_path, file_name);
        file_name_with_path[strlen(FILE_PATH)+strlen(file_name)] = "\0"
        fp = fopen(file_name_with_path, "r");
        read_num = fread(buf, sizeof(char), sizeof(buf), fp);
        fail = false;
        strcpy(start_with_file_name, "start:");
        strcat(start_with_file_name, file_name);
        start_with_file_name[strlen("start:")+strlen(file_name)] = "\0"
        if (send(socket_desc, start_with_file_name, strlen(start_with_file_name), 0) < 0) {
          fail = true;
        }
        while (read_num > 0) {
          if (send(socket_desc, buf, read_num, 0) < 0) {
            fail = true;
            break;
          }
        }
        if (fail) {
          break;
        } else {
          // finish transmission, remove the file
          if (send(socket_desc, "finish", strlen("finish"), 0) < 0) {
            break;
          }
          unlink(file_name_with_path);
        }
      }
      closedir(d);
      fclose(fp);
      close(socket_desc);
    }
  }
  io_thread.join();
}
