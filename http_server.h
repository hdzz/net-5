#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#if __cplusplus < 201100L
#error "Requires C++11 features"
#else

#include "http_request.h"
#include "http_status.h"
#include "serversocket.h"

#include <memory>
#include <string>

namespace net {

namespace internals {

class Handler {
public:
  virtual HttpStatus operator()(const HttpRequest &req) = 0;
};

template<typename T>
class DerivedHandler : public Handler {
public:
  DerivedHandler(const T t) : h(t) { }
  HttpStatus operator()(const HttpRequest& req) {
    return h(req);
  }
private:
  T h;
};

template<typename T>
Handler *make_handler(T t) {
  return new DerivedHandler<T>(t);
}

class HttpHandler {
public:
  template<typename T>
  HttpHandler(const T &f) : h(make_handler(f)) { }
  HttpHandler() = default;

  HttpStatus operator()(const HttpRequest &req) {
    return (*h)(req);
  }

private:
  std::shared_ptr<Handler> h;
};

} // internals

class HttpServer {
public:
  class Options;
  typedef internals::HttpHandler Handler;
  typedef ::std::map<::std::string, Handler> HandlerMap;
  HttpServer(const HandlerMap &handlers, Handler default_handler,
             const Options &options);
private:
  HandlerMap handlers;
  Handler default_handler;
  ServerSocket s;
};

class HttpServer::Options {

};

} // net

#endif // __cplusplus >= 201100L

#endif // HTTP_UTILS_H
