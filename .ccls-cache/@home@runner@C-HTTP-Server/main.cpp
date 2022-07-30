#include "http.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
void FileResponse(Request *req, std::string path) {
  if (auto input = std::ifstream{path}) {
    auto hasExt = [&path](std::string ext) {
      return path.substr(path.size() - ext.size()) == ext;
    };
    if (hasExt(".html")) {
      req->ResponseHeaders.Set("Content-Type", "text/html; charset=UTF-8");
    } else if (hasExt(".js")) {
      req->ResponseHeaders.Set("Content-Type",
                               "text/javascript; charset=UTF-8");
    } else if (hasExt(".css")) {
      req->ResponseHeaders.Set("Content-Type", "text/css; charset=UTF-8");
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    req->Respond(200, buffer.str());
  } else {
    req->Respond(404, std::string{"Not Found: "} + path);
  }
}

} // namespace

int main() {
  Server server = Server();

  server.Get("/", [](Request *req) {
    FileResponse(req, "website/index.html"); // automate this?
  });

  // Static content
  server.Get("/...", [](Request *req) {
    FileResponse(req, "website" + req->HttpHeaders.Path);
  });

  std::cout << "Starting Server...\n";
  server.Start(3000).join();
}