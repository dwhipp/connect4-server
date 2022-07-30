#include "http.hpp"

#include <dirent.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

using std::cout;
using std::string;
using std::vector;

void Headers::Set(string name, string value) {
  Keys.push_back(name);
  Values.push_back(value);
}

string Headers::Get(string name) {
  for (int i = 0; i <= Keys.size(); i++) {
    if (Keys[i] == name) {
      return Values[i];
    }
  }

  return {};
}

Headers::Headers() {
  Keys = vector<string>();
  Values = vector<string>();
}

void Request::Respond(int status, string data) {
  std::ostringstream stream;

  stream << "HTTP/1.1 " << status << " OK\n";

  ResponseHeaders.Set("Content-Length", std::to_string(data.length()));

  for (int i = 0; i < ResponseHeaders.Keys.size(); i++) {
    stream << ResponseHeaders.Keys[i] << ": " << ResponseHeaders.Values[i]
           << "\n";
  }

  stream << "\n" << data;

  (void)write(Socket, stream.str().c_str(), stream.str().length());

  close(Socket);

  Responded = true;
}

Request::Request(int fd, Headers headers, string body) {
  Socket = fd;
  HttpHeaders = headers;
  ResponseHeaders = Headers();

  Responded = false;

  Body = body;
}

Headers Server::ParseHeaders(string data) {
  Headers headers = Headers();
  string curname = "";
  string curvalue = "";
  bool name = true;

  headers.Method = "";
  headers.Path = "";
  bool method = true;

  int headerloc{0};

  // Read request info (Method, Path, HTTP Version)
  for (std::size_t i = 0; i < data.length(); i++) {
    if (data[i] == ' ') {
      if (!method) {
        headerloc = i + 1;
        break;
      }

      method = false;
      continue;
    }

    if (method) {
      headers.Method += data[i];
    } else {
      headers.Path += data[i];
    }
  }

  for (int i = headerloc; i < data.length(); i++) {
    if (data[i] == '\n') {
      name = true;
      headers.Set(curname, curvalue);
      curname = "";
      curvalue = "";

      headers.BodyStart = i + 1;

      if (data[i + 1] == '\n')
        break;

      continue;
    }

    if (data[i] == ':') {
      name = false;
      continue;
    }

    if (name)
      curname += data[i];
    else
      curvalue += data[i];
  }

  return headers;
}

void Server::Get(string path, HTTPCallback callback) {
  Handlers.push_back({"GET", path, callback});
}

void Server::Post(string path, HTTPCallback callback) {
  Handlers.push_back({"POST", path, callback});
}

std::thread Server::Start(int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    std::cerr << "Unable to open socket.\n";
    throw std::exception();
  }

  sockaddr_in addr;
  in_addr inaddr;

  inaddr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_addr = inaddr;
  addr.sin_port = htons(8080);

  // TCP sockets go into a TIME_WAIT state where they cant be used for about a
  // minute.
  int num = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &num, sizeof(int));

  if (bind(fd, (const sockaddr *)&addr, sizeof(sockaddr_in)) < 0) {
    std::cerr << "Socket failed to bind.\n";
    throw std::exception();
  };

  listen(fd, 20);

  return std::thread{[this, fd]() {
    while (true) {
      sockaddr clientaddr{};
      socklen_t len{};

      int req = accept(fd, &clientaddr, &len);

      string body;
      while (true) {
        char buf[1024];
        int count = read(req, buf, sizeof(buf));
        body += buf;
        if (count != sizeof(buf))
          break;
      }

      Headers info = ParseHeaders(body);

      auto Handler = [this, &info]() -> HTTPCallback {
        auto name = info.Path;
        for (const auto &[method, key, value] : Handlers) {
          if (info.Method != method)
            continue;
          int prefixLength = key.size() - 3;
          if (prefixLength > 0 && key.substr(prefixLength) == "...") {
            auto prefix = key.substr(0, prefixLength);
            if (name.size() >= prefixLength &&
                name.substr(0, prefixLength) == prefix) {
            
              return value;
            }
          } else if (key == name) {
            return value;
          }
        }
        return {};
      };

      // HTTP Request object is created here.
      Request httpreq{
          req, info,
          info.Method != "GET"
              ? body.substr(info.BodyStart, stoi(info.Get("Content-Length")))
              : ""};

      auto callback = Handler();

      if (callback) {
        try {
          callback(&httpreq);
        } catch (...) {
          httpreq.Respond(500, "Error");
        }

        if (!httpreq.Responded) {
          std::cerr << "HTTP Handlers must call the respond method!\n\n";

          throw std::exception();
        }
      } else {
        httpreq.Respond(404, "Not Found");
      }
    }

    close(fd);
  }};
}
