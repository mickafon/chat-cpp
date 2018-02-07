/**
* C++ standard libraries
*/
#include <iostrem>
#include <list>
#include <map>
#include <queue>
#include <cstdlib>

/**
* BOOST libraries
*/
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/asio/ip/tcp.hpp>

/**
* Usefull namespaces
*/
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

/**
* Usefull Typedefs : define boost shared pointer,
* to avoid conflit w/ standard smart pointers
* and to reduce code length
*/
typedef boost::shared_ptr< tcp::socket >            socket_ptr;
typedef boost::shared_ptr< string >                 string_ptr;
typedef map< socket_ptr, string_ptr >               clientMap;
typedef boost::shared_ptr< clientMap >              clientMap_ptr;
typedef boost::shared_ptr< list< socket_ptr > >     clientList_ptr;
typedef boost::shared_ptr< queue< clientMap_ptr > > messageQueue_ptr;

/**
* BOOST Asio I/O services
*/
io_services  service;
boost::mutex mtx;
tcp::acceptor    acceptor( service, tcp::endpoint( tcp::v4(), 8001 ) );
clientList_ptr   clientList( new queue< clientMap_ptr > );
messageQueue_ptr messageQueue( new queue< clientMap_ptr > );

const int bufSize = 1024;
enum sleepLen {
  sml = 100,
  lon = 200
}

/**
* Functions Prototypes
*/
bool clientSentExit( string_ptr );
void disconnectClient( socket_ptr );
void acceptorLoop();
void requestLoop();
void responseLoop();

/**
* MAIN
*/
int main()
{
  boost::thread_group threads;

  threads.create_thread( boost::bind( acceptorLoop ) );
  boost::this_thread::sleep( boost::posix_time::millisec( sleepLen::sml ) );

  threads.create_thread( boost::bind( requestLoop ) );
  boost::this_thread::sleep( boost::posix_time::millisec( sleepLen::sml ) );

  threads.create_thread( boost::bind( responseLoop ) );
  boost::this_thread::sleep( boost::posix_time::millisec( sleepLen::sml ) );

  threads.join_all();

  puts("Appuyez sur une touche pour continuer...");
  getc(stdin);

  return EXIT_SUCCESS;
}

/**
* acceptorLoop Defintion
*/
void acceptorLoop()
{
  cout << "En attente d'utilisateurs ..." << endl;

  for(;;)
  {
    socket_ptr clientSock( new tcp::socket( service ) );
    acceptor.accept( *clientSock );

    cout << " ! Un nouvel utilisateur s'est connecté ";

    mtx.lock();
    clientList->emplace_back( clientSock );
    mtx.unlock();

    if( clientList->size() == 1)
    {
      cout << clientList->size() << " client est actuellement connecté" << endl;
    }
    else
    {
      cout << clientList->size() << " clients sont actuellement connectés" << endl;
    }
  }
}

/**
* requestLoop Definition
*/
void requestLoop()
{
  for(;;)
  {
    if( !clientList->empty() )
    {
      // Poorly designed loop, client sockets
      // should alert the server when they have new messages;
      // the server shouldn't poll the clientList while holding a lock
      mtx.lock();

      for( auto& clientSock : *clientList )
      {
        if( clientSock->available() )
        {
          char readBuf[bufSize] = { 0 };
          int bytesRead         = clientSock->read_some(buffer( readBuf, bufSize ) );
          string_ptr msg( new string( readBuf, bytesRead ) );

          if( clientSentExit( msg ) )
          {
            disconnectClient( clientSock );
            break;
          }

          clientMap_ptr cm( new clientMap );
          cm->insert( pair< socket_ptr, string_ptr >( clientSock, msg ) );
          messageQueue->push( cm );

          cout << "ChatLog: " << *msg << endl;
        }
      }
      mtx.unlock();
    }
    boost::this_thread::sleep( boost::posix_time::millisec( sleepLen::lon ) );
  }
}

/**
* clientSentExit Definition (requestLoop)
*/
bool clientSentExit( string_ptr message )
{
  if( message->find( "exit" ) != string::npos )
  {
    return true;
  }

  else
  {
    return false;
  }

}

/**
* disconnectClient Defintion (requestLoop)
*/
void disconnectClient( socket_ptr clientSock )
{
  auto position = find( clientList->begin(), clientList->end(), clientSock );

  clientSock->shutdown( tcp::socket::shutdown_both );
  clientSock->close();
  clientList->erase( position );

  cout << "! Un client s'est déconnecté " << endl;
  cout << clientList->size() << " clients sont actuellement connectés" << endl;
}

/**
* responseLoop Definition
*/
void responseLoop()
{

  for(;;)
  {

    if( !messageQueue->empty() )
    {

      auto message = messageQueue->front();
      mtx.lock();

      for( auto& clientSock : *clientList )
      {
        clientSock->write_some( buffer( *( message->begin()->second ), bufSize ) );
      }

      mtx.unlock();
      mtx.lock();
      messageQueue->pop();
      mtx.unlock();
    }
    boost::this_thread::sleep( boost::posix_time::millisec( sleepLen::lon ) );
  }
}
