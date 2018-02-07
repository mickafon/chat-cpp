/**
* C++ standard libraries
*/
#include <iostream>
#include <string>
#include <queue>
#include <cstdlib>

/**
* BOOST libraries
*/
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string.hpp>

/**
* Usefull namespaces
*/
using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

/**
* Usefull Typedefs : define boost shared pointer,
* to avoid conflit w/ standard smart pointers
* and to reduce code length
*/
typedef boost::shared_ptr< tcp::socket >         socket_ptr;
typedef boost::shared_ptr< string >              string_ptr;
typedef boost::shared_ptr< queue< string_ptr > > messageQueue_ptr;

/***
* BOOST Asio I/O service
*/
io_service       service;
messageQueue_ptr messageQueue( new queue< string_ptr > );
tcp::endpoint    ep( ip::address::from_string( "127.0.0.1" ), 8001 );
const int        inputSize = 256;
string_ptr       promptCpy;

/**
* Functions Prototypes
*/
bool    isOwnMessage( string_ptr );
void    displayLoop( socket_ptr );
void    inboundLoop( socket_ptr, string_ptr );
void    writeLoop( socket_ptr, string_ptr );
string* buildPrompt();

/**
* MAIN
*/
int main(int argc, char** argv)
{
  try
  {
    boost::thread_group threads;
    socket_ptr          sock( new tcp::socket(service));

    string_ptr prompt( buildPrompt() );
    promptCpy = prompt;

    sock->connect( ep );

    cout << "Bienvenue sur le Chat Unicorn \n Entrez 'exit' pour sortir" << endl;

    threads.create_thread( boost::bind( displayLoop, sock ) );
    threads.create_thread( boost::bind( inboundLoop, sock, prompt ) );
    threads.create_thread( boost::bind( writeLoop,   sock, prompt ) );

    threads.join_all();
  }
  catch( std::exception&amp; e )
  {
    cerr << e.what() << endl;
  }

  puts( " Appuyez sur une touche pour continuer ..." );
  gets( stdin );

  return EXIT_SUCCESS;
}

/**
* buildPrompt Defintion
*/
string* buildPrompt()
{
  const int inputSize           = 256;
  char      inputBuf[inputSize] = { 0 };
  char      nameBuf[inputSize]  = { 0 };

  string* prompt = new string(": ");

  cout << "Saisissez un nom d'utilisateur :  ";
  cin.getline( nameBuf, inputSize );

  *prompt = ( string )nameBuf + *prompt;
  boost::algorithm::to_lower( *prompt );

  return prompt;
}

/**
* inboundLoop Definition
*/
void inboundLoop( socket_ptr sock,
                  string_ptr prompt )
{
  int bytesRead      = 0;
  char readBuf[1024] = { 0 };

  for(;;)
  {
    if( sock->available() )
    {
      bytesRead = sock->read_some( buffer( readBuf, inputSize ) );
      string_ptr msg( new string( readBuf, bytesRead ) );
      messageQueue->push(msg);
    }
    boost::this_thread::sleep( boost::posix_time::millisec( 1000 ) );
  }
}

/**
* writeLoop Definition
*/
void writeLoop( socket_ptr sock,
                string_ptr prompt )
{
  char inputBuf[inputSize] = {0};
  string inputMsg;

  for(;;)
  {
    cin.getline( inputBuf, inputSize );
    inputMsg = *prompt + (string)inputBuf + '\n';

    if( !inputMsg.empty() )
    {
      sock->write_some( buffer( inputMsg, inputSize ) );
    }

    if( inputMsg.find( "exit" ) != string::npos )
    {
      exit(1);
    }

    inputMsg.clear();
    memset( inputBuf, 0, inputSize );
  }
}

/**
* displayLoop Definition
*/
void displayLoop( socket_ptr sock )
{
  for(;;)
  {
    if( !messageQueue->empty() )
    {
      // Can you refactor this code to handle multiple users with the same prompt?
      if( !isOwnMessage( messageQueue->front() ) )
      {
        cout << "\n" + *( messageQueue->front() );
      }
      messageQueue->pop();
    }
    boost::this_thread::sleep( boost::posix_time::millisec( 1000 ) );
  }
}

/**
* isOwnMessage Definition
*/
bool isOwnMessage( string_ptr message )
{
  if( message->find( *promptCpy ) != string::npos )
  {
    return true;
  }
  else
  {
    return false;
  }
}
