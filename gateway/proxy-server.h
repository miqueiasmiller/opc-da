#pragma once

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <functional>
#include <memory>
#include <string>

#include "opc_utils.h"

static unsigned const MAX_LENGTH = 100;
static unsigned const POOL_SIZE = 30;

class ProxyServer
{
public:
  ProxyServer(int const &, function<string(string const & itemId)>, function<bool(string const & itemId, string value)>);
  ~ProxyServer();
  void start();

private:
  unsigned available_connections;
  boost::asio::io_service tcp_service;
  boost::asio::io_service worker_service;
  boost::asio::ip::tcp::acceptor tcp_acceptor;
  boost::asio::io_service::work worker;
  boost::condition_variable cv;
  boost::mutex m;
  boost::thread_group threadpool;

  function<string(string const & itemId)> readFunc;
  function<bool(string const & itemId, string value)> writeFunc;

  void session(boost::asio::ip::tcp::socket *);
  std::string read_request(boost::asio::ip::tcp::socket *);
  void process_request(boost::asio::ip::tcp::socket *, std::string const &);
  void start_threadpool();
  void check_connections_pool();
};
