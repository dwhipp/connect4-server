#include "http.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <map>
#include <cstdio>

#include "connect4/src-cc/Board.h"
#include "connect4/src-cc/Player.h"

namespace {
void FileResponse(Request *req, std::string path) {
  if (auto input = std::ifstream{path}) {
    auto hasExt = [&path](std::string ext) {
      return path.size() > ext.size() &&
        path.substr(path.size() - ext.size()) == ext;
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

void PlayMcts(Request* req) {
  auto path = req->HttpHeaders.Path;
  std::map<std::string, std::string> param;
  auto param_str = path.substr(path.find('?') + 1);
  for(int i = 0; i < 10; i++) {
    auto eq = param_str.find('=');
    auto end = param_str.find('&');
    if (eq < end) {
      param[param_str.substr(0, eq)] = param_str.substr(eq+1, end-eq-1);
    } else {
      param[param_str.substr(0, end)] = "";
    }
    if (end == param_str.npos) break;
    param_str = param_str.substr(end+1);
  }
  req->ResponseHeaders.Set("Content-Type",
                           "application/json; charset=UTF-8");
  if (param["player"].empty()) {
    req->Respond(200, "{ \"error\": \"no-player\" }");
    return;
  }
  if (param["states"].empty()) {
    req->Respond(200, "{ \"error\": \"no-states\" }");
    return;
  }
  if (param["board"].empty()) {
    req->Respond(200, "{ \"error\": \"no-board\" }");
    return;
  }

  
  auto board = Board::New();
  auto p = param["player"];
  auto b = param["board"];
  auto s = param["states"];
 // std::cout << p << "," << s << "," b << "\n";
  for (int i = 0; i < b.size(); i++) {
    int col = i % 7;
    if (b[i] == s[0]) continue;
    bool player_enum = false;
    if (b[i] == s[1]) { player_enum = false; }
    else if (b[i] == s[2]) { player_enum = true; }
    else {
      req->Respond(200, "{ \"error\": \"invalid-board\" }");
      return;
    }
    if (board->PlayStone(player_enum, col)) {
      req->Respond(200, "{ \"error\": \"game over\" }");
      return;
    }
  }
  auto player = Player::New(p);
  bool active_player = player->name() == s.substr(2);
  std::cout << "PLAYER=" << player->name() << "\n";
  player->StartGame(board.get(), active_player);
  try {
    auto move = player->GetMove();
    bool result = board->PlayStone(active_player, move);
    char buffer[100];
    std::sprintf(buffer, "{ \"column\": %u, \"result\" : %u }", move, result);
    std::cout << "resposne: " << buffer << "\n";
    req->Respond(200, buffer);
  } catch (...) {
    req->Respond(200, "{ \"error\": \"no valid move\" }");
  }
}

} // namespace

int main() {
  Server server = Server();

  server.Get("/", [](Request *req) {
    FileResponse(req, "website/index.html"); // automate this?
  });

  server.Get("/api/mcts?...", PlayMcts);

  // Static content
  server.Get("/...", [](Request *req) {
    FileResponse(req, "website" + req->HttpHeaders.Path);
  });

  std::cout << "Starting Server...\n";
  server.Start(3000).join();
}