#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <fcntl.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <queue>
#include <array>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HttpService.h"
#include "HttpUtils.h"
#include "FileService.h"
#include "MySocket.h"
#include "MyServerSocket.h"
#include "dthread.h"

using namespace std;

int PORT = 8080;
int THREAD_POOL_SIZE = 1;
int BUFFER_SIZE = 1;
bool DEBUG = false;
string BASEDIR = "static";
string SCHEDALG = "FIFO";
string LOGFILE = "/dev/null";

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
/// Broadcasted when new connection enters buffer
pthread_cond_t got_conn = PTHREAD_COND_INITIALIZER;
/// Broadcasted when connection starts being handled
pthread_cond_t handled_conn = PTHREAD_COND_INITIALIZER;

/// Debug print `msg`. Does not do anything if `DEBUG` is not set (by `-g` flag)
void debug(string src, string msg) {
  if (!DEBUG)
    return;
  cout << src << ": " << msg << endl;
}

vector<HttpService *> services;

HttpService *find_service(HTTPRequest *request) {
   // find a service that is registered for this path prefix
  for (unsigned int idx = 0; idx < services.size(); idx++) {
    if (request->getPath().find(services[idx]->pathPrefix()) == 0) {
      return services[idx];
    }
  }

  return NULL;
}


void invoke_service_method(HttpService *service, HTTPRequest *request, HTTPResponse *response) {
  stringstream payload;

  // invoke the service if we found one
  if (service == NULL) {
    // not found status
    response->setStatus(404);
  } else if (request->isHead()) {
    service->head(request, response);
  } else if (request->isGet()) {
    service->get(request, response);
  } else {
    // The server doesn't know about this method
    response->setStatus(501);
  }
}

void handle_request(MySocket *client) {
  HTTPRequest *request = new HTTPRequest(client, PORT);
  HTTPResponse *response = new HTTPResponse();
  stringstream payload;
  
  // read in the request
  bool readResult = false;
  try {
    payload << "client: " << (void *) client;
    sync_print("read_request_enter", payload.str());
    readResult = request->readRequest();
    sync_print("read_request_return", payload.str());
  } catch (...) {
    // swallow it
  }    
    
  if (!readResult) {
    // there was a problem reading in the request, bail
    delete response;
    delete request;
    sync_print("read_request_error", payload.str());
    return;
  }
  
  HttpService *service = find_service(request);
  invoke_service_method(service, request, response);

  // send data back to the client and clean up
  payload.str(""); payload.clear();
  payload << " RESPONSE " << response->getStatus() << " client: " << (void *) client;
  sync_print("write_response", payload.str());
  cout << payload.str() << endl;
  client->write(response->response());
    
  delete response;
  delete request;

  payload.str(""); payload.clear();
  payload << " client: " << (void *) client;
  sync_print("close_connection", payload.str());
  client->close();
  delete client;
}

/// FIFO connection buffer class.
///
/// Notably does not have multithread control
class ConnBuf {
  public:
    ConnBuf() {}
    ConnBuf(int max_buf_size) {
      buf_size = max_buf_size;
    }

    void set_bufsize(int max_buf_size) {
      if (sockets.size() > buf_size)
        throw invalid_argument("max_buf_size is too small");
      buf_size = max_buf_size;
    }

    bool is_full() {
      bool ret = sockets.size() >= buf_size;
      return ret;
    }

    bool is_empty() {
      return sockets.empty();
    }

    void enqueue(MySocket* socket) {
      if (is_full())
        throw invalid_argument("ConnBuf is full");
      sockets.push(socket);
    }

    /// Return earliest socket inserted and pop it
    MySocket* dequeue() {
      MySocket* ret = sockets.front();
      sockets.pop();
      return ret;
    }
  private:
    size_t buf_size;
    queue<MySocket*> sockets;
};
ConnBuf conn_buf;

/// Start routine of a worker thread
void* worker(void* _args) {
  dthread_mutex_lock(&lock);
  while (true) {
    while (conn_buf.is_empty()) {
      dthread_cond_wait(&got_conn, &lock);
    }
    MySocket* client = conn_buf.dequeue();
    dthread_cond_broadcast(&handled_conn);

    dthread_mutex_unlock(&lock);
    handle_request(client);
    dthread_mutex_lock(&lock);
  }
  dthread_mutex_unlock(&lock);
  return NULL;
}


int main(int argc, char *argv[]) {

  signal(SIGPIPE, SIG_IGN);
  int option;

  while ((option = getopt(argc, argv, "d:p:t:b:s:l:g")) != -1) {
    switch (option) {
    case 'd':
      BASEDIR = string(optarg);
      break;
    case 'p':
      PORT = atoi(optarg);
      break;
    case 't':
      THREAD_POOL_SIZE = atoi(optarg);
      break;
    case 'b':
      BUFFER_SIZE = atoi(optarg);
      break;
    case 's':
      SCHEDALG = string(optarg);
      break;
    case 'l':
      LOGFILE = string(optarg);
      break;
    case 'g':
      DEBUG = true;
      break;
    default:
      cerr<< "usage: " << argv[0] << " [-p port] [-t threads] [-b buffers]" << endl;
      exit(1);
    }
  }

  set_log_file(LOGFILE);

  sync_print("init", "");
  MyServerSocket *server = new MyServerSocket(PORT);
  MySocket *client;

  // The order that you push services dictates the search order
  // for path prefix matching
  services.push_back(new FileService(BASEDIR));

  // Thread pooling
  unique_ptr<pthread_t[]> thread_pool(new pthread_t[THREAD_POOL_SIZE]);
  for (int i = 0; i < THREAD_POOL_SIZE; i++) {
    if (dthread_create(&thread_pool[i], NULL, &worker, NULL)) {
      cerr << "failed to create thread" << endl;
      return 1;
    }
    debug("main", "Created thread " + to_string(thread_pool[i]));
  }

  // Buffer
  conn_buf.set_bufsize(BUFFER_SIZE);

  dthread_mutex_lock(&lock);
  while(true) {
    dthread_mutex_unlock(&lock);
    sync_print("waiting_to_accept", "");
    debug("main", "waiting_to_accept\n");
    client = server->accept();
    sync_print("client_accepted", "");
    debug("main", "client_accepted\n");
    dthread_mutex_lock(&lock);

    while (conn_buf.is_full()) {
      dthread_cond_wait(&handled_conn, &lock);
    }

    conn_buf.enqueue(client);
    dthread_cond_broadcast(&got_conn);
  }
  dthread_mutex_unlock(&lock);
}
