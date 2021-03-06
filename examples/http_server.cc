#include "../fileserver.h"
#include "../http_server.h"
#include <cstdlib>
#include <map>
#include <string>
#include <iostream>

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " port" << std::endl;
    return 1;
  }
  net::HttpServer::HandlerConfiguration m = {
    {"/", {
      [] (const net::HttpRequest& r) {
        return net::HttpStatus(200,"OK",{}, "Index page");
      },
      false
    }},
    {"/path1", {
      [] (const net::HttpRequest& r) {
        return net::HttpStatus(418,"NOT A TEAPOT",{}, "TEAPOT");
      },
      true
      }},
    {"/path2", {
      [] (const net::HttpRequest& r) {
        return net::HttpStatus(200, "OK", {}, "Congratulations!");
      },
      false
      }},
    net::FileServer::handle_path("/src", "./")
  };

  auto def = [](const net::HttpRequest& r) {
    return net::HttpStatus(404, "NOT FOUND", {}, "404 not found");
  };

  net::HttpServer::Options opts;
  opts.port = atoi(argv[1]);

  net::HttpServer http_server(m, def, opts);

  auto res = http_server.start();

  std::cout << "Finished serving; result was " << res.get() << std::endl;
}
