#include "proxy-server.h"

ProxyServer::ProxyServer(int const & port, function<string(string const & itemId)>readFnHandler, function<bool(string const & itemId, string value)> writeFnHandler, function<string()> changedFnHandler) :
tcp_service(),
worker_service(),
tcp_acceptor(tcp_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
worker(worker_service),
readFunc(readFnHandler),
writeFunc(writeFnHandler),
changedFunc(changedFnHandler)
{
  available_connections = POOL_SIZE;

  start_threadpool();

  std::cout << "Proxy Server - Listening on " << tcp_acceptor.local_endpoint() << std::endl;
}


ProxyServer::~ProxyServer()
{
  // para o loop serviço para que nenhuma tarefa após esse ponto seja executada.
  worker_service.stop();

  tcp_service.stop();

  // aguarda o fim das tarefas em execução
  threadpool.join_all();
}


void ProxyServer::start_threadpool()
{
  //std::cout << "Application Server - Loading Thread Pool - Pool size = " << POOL_SIZE << std::endl;

  for (unsigned i = 0; i < POOL_SIZE; i++)
    threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &worker_service));
}


void ProxyServer::start()
{
  while (true)
  {
    check_connections_pool();

    boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket(tcp_service);
    tcp_acceptor.accept(*socket);

    std::cout << "Application Server - New conection from " << socket->remote_endpoint() << std::endl;

    worker_service.post(boost::bind(&ProxyServer::session, this, socket));
  }
}


void ProxyServer::check_connections_pool()
{
  boost::unique_lock<boost::mutex> lock(m);
  while (available_connections <= 0)
  {
    std::cout << "Application Server - Maximun number of connections reach" << std::endl;
    cv.wait(lock);
  }

  --available_connections;
}


void ProxyServer::session(boost::asio::ip::tcp::socket* socket)
{
  try
  {
    while (true)
    {
      std::string message = read_request(socket);
      boost::algorithm::trim(message);

      if (message == "DISCONNECTED")
      {
        std::cerr << "Application Server - Client has disconnected." << socket->remote_endpoint() << std::endl;
        break;
      }

      process_request(socket, message);
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "Application Server - Session error - " << e.what() << std::endl;
  }

  if (socket->is_open())
    socket->close();

  // libera uma conexão para o pool...
  boost::unique_lock<boost::mutex> lock(m);
  ++available_connections;
  cv.notify_one();
}


std::string ProxyServer::read_request(boost::asio::ip::tcp::socket * socket)
{
  boost::system::error_code error;
  char data[MAX_LENGTH];

  size_t length = socket->read_some(boost::asio::buffer(data, MAX_LENGTH), error);

  if (error == boost::asio::error::eof) // Client has disconnected
    return "DISCONNECTED";
  else if (error)
    throw boost::system::system_error(error); // Some other error.

  return std::string(data, length);
}


void ProxyServer::process_request(boost::asio::ip::tcp::socket * socket, std::string const & message)
{
  vector<string>tokens;

  tokenizer(const_cast<string &>(message), tokens);

  if (tokens.size() > 0)
  {
    if (tokens[0] == "READ" && tokens.size() > 1)
    {
      std::cout << "Application Server - Message received from " << socket->remote_endpoint() << " - Message: " << message << std::endl;

      string res = readFunc(tokens[1]);
      socket->write_some(boost::asio::buffer(res));
    }
    else if (tokens[0] == "WRITE" && tokens.size() > 2)
    {
      std::cout << "Application Server - Message received from " << socket->remote_endpoint() << " - Message: " << message << std::endl;

      bool res = writeFunc(tokens[1], tokens[2]);
      socket->write_some(boost::asio::buffer(res ? string("WRITE_OK") : string("WRITE_FAIL")));
    }
    else if (tokens[0] == "GETCHANGED")
    {
      string res = changedFunc();
      socket->write_some(boost::asio::buffer(res));
    }
    else
    {
      std::cout << "Application Server - Message received from " << socket->remote_endpoint() << " - Message: " << message << std::endl;

      socket->write_some(boost::asio::buffer(string("INVALID")));
    }
  }
}

void ProxyServer::tokenizer(std::string & message, std::vector<std::string> & tokens)
{
  std::string delimiter = "|";

  size_t pos = 0;
  std::string tok;

  tokens.clear();

  while ((pos = message.find(delimiter)) != std::string::npos)
  {
    tok = message.substr(0, pos);
    tokens.push_back(tok);
    message.erase(0, pos + delimiter.length());
  }

  if (message.length() > 0)
    tokens.push_back(message);
}