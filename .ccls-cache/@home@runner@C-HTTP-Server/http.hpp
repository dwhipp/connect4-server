#include <functional>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

class Headers {
public:
  std::vector<std::string> Keys;
  std::vector<std::string> Values;

  std::string Method;
  std::string Path;

  // Where the HTTP body starts
  int BodyStart;

  void Set(std::string name, std::string value);
  std::string Get(std::string name);
  Headers();
};

class Request {
  int Socket;

public:
  bool Responded;

  Headers HttpHeaders;
  Headers ResponseHeaders;

  std::string Body;

  void Respond(int status, std::string data);
  Request(int fd, Headers headers, std::string body);
};

class Server {
  using HTTPCallback = std::function<void(Request *)>;

public:
  bool Running{false};

  void Get(std::string path, HTTPCallback callback);

  void Post(std::string path, HTTPCallback callback);

  std::thread Start(int port);

private:
  std::vector<std::tuple<std::string, std::string, HTTPCallback>> Handlers;

  Headers ParseHeaders(std::string data);
};