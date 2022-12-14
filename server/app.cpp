#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string>
#include <sstream>
#include <fstream>
#include "uri.cpp"
#include "compress.cpp"

#define PORT 8080

int main() {
  int server_fd, new_socket;
  long valread;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("In socket");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( PORT );

  memset(address.sin_zero, '\0', sizeof address.sin_zero);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
    perror("In bind");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 10) < 0) {
    perror("In listen");
    exit(EXIT_FAILURE);
  }

  while(1) {
    std::cout << "\n+++++++ Waiting for new connection ++++++++\n\n";
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
      perror("In accept");
      exit(EXIT_FAILURE);
    }
    
    char buffer[30000] = {0};
    valread = read( new_socket , buffer, 30000);
    printf("%s\n",buffer );
    std::istringstream f(buffer);
    std::string line;
    std::string URL = "";

    while (std::getline(f, line)) {
      if (line.rfind("GET", 0) == 0) {
        std::string str = "GET /";
        std::string str2 = "HTTP/1.1";
        URL = line.substr(str.length(), line.length() - str.length());
        URL = URL.substr(0, line.length() - str2.length() - 7);
        break;
      }
    }

    // Uri uri1 = Uri::Parse(L"http://localhost:8080/" + std::wstring(URL.begin(), URL.end()));

    if (URL == "favicon.ico") {
      close(new_socket);
      continue;
    }

    std::string response_s = "";

    std::string content_type = "text/plain";

    if (URL.find(".") == std::string::npos) {
      // serve HTML if no file extension
      std::ifstream t((URL == "" ? "index" : URL)+".html", std::ios::in | std::ios::binary);
      std::string html( (std::istreambuf_iterator<char>(t) ), (std::istreambuf_iterator<char>()));
      response_s = html;
      content_type = "text/html";
    } else {
      // static serving
      std::ifstream t(URL, std::ios::in | std::ios::binary);
      std::string content( (std::istreambuf_iterator<char>(t) ), (std::istreambuf_iterator<char>()));
      response_s = content;
    }

    std::string response = (
      "HTTP/1.1 200 OK\n"
      "Cache-Control: max-age=120\n"
      "Content-Type: " + content_type + "\n"
      "Content-Encoding: deflate\n"
      "Content-Length: " + std::to_string(response_s.length()) + "\n\n"
      + compress_string(response_s)
    );

    char * hello = (char *)response.data();

    write(new_socket , hello , response.size());
    std::cout << "------------------message sent-------------------";;
    close(new_socket);
  }
  return 0;
}