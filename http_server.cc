#if __cplusplus >= 201100L

#include "http_request.h"
#include "http_server.h"
#include "http_status.h"
#include "path.h"
#include "net_error.h"
#include "serversocket.h"
#include "socket.h"

#include <future>
#include <thread>

namespace net {

namespace internals {

namespace {

// Trie to allow for partial matching of paths.
struct Node {
  const static Handler null_handler;
  Node(Handler h = null_handler, bool b = false)
    : handler(h), capture_children(b) {}
  Handler handler;
  bool capture_children;
  ::std::map<::std::string, Node> next;
};

// Analogous to a null pointer; offers some default behaviour.
const Handler Node::null_handler = {
  [](HttpRequest r) {
    return net::HttpStatus(500, "Internal Server Error", {},
                           "500 Internal Server Error");
  }
};

} // namespace

// Class that maps paths to handlers.
// Allows for partial matching. See HandlerConfiguration for more details.
class HandlerMap {
public:
  HandlerMap(const HttpServer::HandlerConfiguration &config,
             Handler default_handler);
  Handler get_handler(Query query) const;
private:
  Node root;
  Handler default_handler;
};

HandlerMap::HandlerMap(const HttpServer::HandlerConfiguration &config,
                       Handler default_handler)
  : root(Node::null_handler, false), default_handler(default_handler)
{
  // Insert each handler into the trie
  for (const auto &conf : config) {
    Query query = Query(conf.first);
    Node *n = &root;
    for (const auto &s : query.components) {
      n = &(n->next[s]);
    }
    n->handler = conf.second.first;
    n->capture_children = conf.second.second;
  }
}

Handler HandlerMap::get_handler(Query query) const
{
  Handler h = (root.capture_children || !query.components.size()) ?
      root.handler : default_handler;
  const internals::Node *n = &root;
  for (unsigned i = 0; i < query.components.size(); i++) {
    auto next = n->next.find(query.components[i]);
    if (next == n->next.end()) {
      return h;
    }
    n = &(next->second);
    // If our node captures children / if it's the last node, keep the handler.
    if (&n->handler != &Node::null_handler
        && (n->capture_children || i == query.components.size() - 1)) {
      h = n->handler;
    }
  }
  return h;
}

} // internals

HttpServer::HttpServer(const HandlerConfiguration &handlers,
                       Handler default_handler,
                       const Options &options)
    : ss(options.port, options.queue_length)
{
  this->handlers = new internals::HandlerMap(handlers, default_handler);
  die_on_error = options.die_on_error;
  err = ss.error();
}

HttpServer::~HttpServer()
{
  delete handlers;
}

Error HttpServer::error() const {
  return err;
}

::std::future<Error> HttpServer::start() {
  return ::std::async([=]() {
    if (!ss) return ss.error();

    for (;;) {
      Socket s = ss.accept();

      if (s) {
        // Start up a thread to handle the request
        ::std::thread([=]() {
          HttpRequest request;

          request.read_from_socket(s);

          Handler h = handlers->get_handler(request.query);

          HttpStatus response = h(request);

          response.write_to_socket(s);
        }).detach();
      } else if (die_on_error) {
        return s.error();
      }
    }
    return OK;
  });
}

} // net

#endif // __cplusplus >= 201100L
