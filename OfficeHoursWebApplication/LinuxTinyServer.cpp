// Linux tiny HTTP server.
// Nicole Hamilton  nham@umich.edu

// This variation of LinuxTinyServer supports a simple plugin interface
// to allow "magic paths" to be intercepted.

// Usage:  LinuxTinyServer port rootdirectory

// Compile with g++ -pthread LinuxTinyServer.cpp -o LinuxTinyServer
// To run under WSL (Windows Subsystem for Linux), must elevate with
// sudo, otherwise bind will fail.

// LinuxTinyServer does not look for default index.htm or similar
// files.  If it receives a GET request on a directory, it will refuse
// it, returning an HTTP 403 error, access denied.  This could be
// improved.

// It also does not support HTTP Connection: keep-alive requests and
// will close the socket at the end of each response.  This is a
// perf issue, forcing the client browser to reconnect for each
// request and a candidate for improvement.


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <cassert>
using namespace std;


 // The constructor for any plugin should set Plugin = this so that
 // LinuxTinyServer knows it exists and can call it.

#include "Plugin.h"
PluginObject *Plugin = nullptr;


// Root directory for the website, taken from argv[ 2 ].
// (Yes, a global variable since it never changes.)

char *RootDirectory;


//  Multipurpose Internet Mail Extensions (MIME) types

struct MimetypeMap
   {
   const char *Extension, *Mimetype;
   };

const MimetypeMap MimeTable[ ] =
   {
   // List of some of the most common MIME types in sorted order.
   // https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Complete_list_of_MIME_types
   ".3g2",     "video/3gpp2",
   ".3gp",     "video/3gpp",
   ".7z",      "application/x-7z-compressed",
   ".aac",     "audio/aac",
   ".abw",     "application/x-abiword",
   ".arc",     "application/octet-stream",
   ".avi",     "video/x-msvideo",
   ".azw",     "application/vnd.amazon.ebook",
   ".bin",     "application/octet-stream",
   ".bz",      "application/x-bzip",
   ".bz2",     "application/x-bzip2",
   ".csh",     "application/x-csh",
   ".css",     "text/css",
   ".csv",     "text/csv",
   ".doc",     "application/msword",
   ".docx",    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
   ".eot",     "application/vnd.ms-fontobject",
   ".epub",    "application/epub+zip",
   ".gif",     "image/gif",
   ".htm",     "text/html",
   ".html",    "text/html",
   ".ico",     "image/x-icon",
   ".ics",     "text/calendar",
   ".jar",     "application/java-archive",
   ".jpeg",    "image/jpeg",
   ".jpg",     "image/jpeg",
   ".js",      "application/javascript",
   ".json",    "application/json",
   ".mid",     "audio/midi",
   ".midi",    "audio/midi",
   ".mpeg",    "video/mpeg",
   ".mpkg",    "application/vnd.apple.installer+xml",
   ".odp",     "application/vnd.oasis.opendocument.presentation",
   ".ods",     "application/vnd.oasis.opendocument.spreadsheet",
   ".odt",     "application/vnd.oasis.opendocument.text",
   ".oga",     "audio/ogg",
   ".ogv",     "video/ogg",
   ".ogx",     "application/ogg",
   ".otf",     "font/otf",
   ".pdf",     "application/pdf",
   ".png",     "image/png",
   ".ppt",     "application/vnd.ms-powerpoint",
   ".pptx",    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
   ".rar",     "application/x-rar-compressed",
   ".rtf",     "application/rtf",
   ".sh",      "application/x-sh",
   ".svg",     "image/svg+xml",
   ".swf",     "application/x-shockwave-flash",
   ".tar",     "application/x-tar",
   ".tif",     "image/tiff",
   ".tiff",    "image/tiff",
   ".ts",      "application/typescript",
   ".ttf",     "font/ttf",
   ".vsd",     "application/vnd.visio",
   ".wav",     "audio/x-wav",
   ".weba",    "audio/webm",
   ".webm",    "video/webm",
   ".webp",    "image/webp",
   ".woff",    "font/woff",
   ".woff2",   "font/woff2",
   ".xhtml",   "application/xhtml+xml",
   ".xls",     "application/vnd.ms-excel",
   ".xlsx",    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
   ".xml",     "application/xml",
   ".xul",     "application/vnd.mozilla.xul+xml",
   ".zip",     "application/zip"
   };


const char *Mimetype( const string filename )
   {
   // Return the Mimetype associated with any extension on the filename.

   const char *begin = filename.c_str( ),
      *p = begin + filename.length( ) - 1;

   // Scan back from the end for an extension.
   while ( p >= begin && *p != '.' )
      p--;

   if ( *p == '.' )
      {
      // Found an extension.  Binary search for a matching mimetype.

      int i = 0, j = sizeof( MimeTable )/sizeof( MimetypeMap ) - 1;
      while ( i <= j )
         {
         int mid = ( i + j )/2,
            compare = strcasecmp( p, MimeTable[ mid ].Extension );
         if ( compare == 0 )
            return MimeTable[ mid ].Mimetype;
         if ( compare < 0 )
            j = mid - 1;
         else
            i = mid + 1;
         }
      }

   // Anything not matched is an "octet-stream", treated as
   // an unknown binary, which browsers treat as a download.

   return "application/octet-stream";
   }


int HexLiteralCharacter( char c )
   {
   // If c contains the Ascii code for a hex character, return the
   // binary value; otherwise, -1.

   int i;

   if ( '0' <= c && c <= '9' )
      i = c - '0';
   else
      if ( 'a' <= c && c <= 'f' )
         i = c - 'a' + 10;
      else
         if ( 'A' <= c && c <= 'F' )
            i = c - 'A' + 10;
         else
            i = -1;

   return i;
   }


string UnencodeUrlEncoding( string &path )
   {
   // Unencode any %xx encodings of characters that can't be
   // passed in a URL.

   // (Unencoding can only shorten a string or leave it unchanged.
   // It never gets longer.)

   const char *start = path.c_str( ), *from = start;
   string result;
   char c, d;


   while ( ( c = *from++ ) != 0 )
      if ( c == '%' )
         {
         c = *from;
         if ( c )
            {
            d = *++from;
            if ( d )
               {
               int i, j;
               i = HexLiteralCharacter( c );
               j = HexLiteralCharacter( d );
               if ( i >= 0 && j >= 0 )
                  {
                  from++;
                  result += ( char )( i << 4 | j );
                  }
               else
                  {
                  // If the two characters following the %
                  // aren't both hex digits, treat as
                  // literal text.

                  result += '%';
                  from--;
                  }
               }
            }
         }
      else
         result += c;

   return result;
   }


bool SafePath( const char *path )
   {
   // Watch out for paths containing .. segments that
   // attempt to go higher than the root directory
   // for the website.

   // Should start with a /.
   if ( *path != '/' )
      return false;

   int depth = 0;
   for ( const char *p = path + 1;  *p;  )
      if ( strncmp( p, "../", 3 ) == 0 )
         {
         depth--;
         if ( depth < 0 )
            return false;
         else
            p += 3;
         }
      else
         {
         for ( p++; *p && *p != '/';  p++ )
            ;
         if ( *p == '/' )
            {
            depth++;
            p++;
            }
         }
   return true;
   }


class RequestHeader
   {
   public:
      string Action;
      string Path;
   };


RequestHeader ParseRequestHeader( const char *request, size_t length )
   {
   // Extract the action and URL unencoded path.

   RequestHeader r;
   const char *p, *eof = request + length;

   // First word is the action.
   for ( p = request;  p < eof && *p != ' ';  p++ )
      ;
   if ( p - request )
      r.Action = string( request, p - request );

   // Skip over any whitespace.
   while ( p < eof && *p == ' ' )
      p++;

   // Beginning of the path.
   const char *path = p;
   while ( p < eof && *p != ' ' )
      p++;

   if ( p > path )
      {
      r.Path = string( path, p - path );
      r.Path = UnencodeUrlEncoding( r.Path );
      }

   return r;
   }


off_t FileSize( int f )
   {
   // Return -1 for directories.

   struct stat fileInfo;
   fstat( f, &fileInfo );
   if ( ( fileInfo.st_mode & S_IFMT ) == S_IFDIR )
      return -1;
   return fileInfo.st_size;
   }


void AccessDenied( int talkSocket )
   {
   const char accessDenied[ ] = "HTTP/1.1 403 Access Denied\r\n"
         "Content-Length: 0\r\n"
         "Connection: close\r\n\r\n";

   cout << accessDenied;
   send( talkSocket, accessDenied, sizeof( accessDenied ) - 1, 0 );
   }

   
void FileNotFound( int talkSocket )
   {
   const char fileNotFound[ ] = "HTTP/1.1 404 Not Found\r\n"
         "Content-Length: 0\r\n"
         "Connection: close\r\n\r\n";

   cout << fileNotFound;
   send( talkSocket, fileNotFound, sizeof( fileNotFound ) - 1, 0 );
   }

               
void *Talk( void *talkSocket )
   {
   // Look for a GET message, then reply with the
   // requested file.

   // Cast from void * to int * to recover the talk socket id
   // as ts and then delete the copy passed on the heap.

   int *p = ( int * )talkSocket, ts = *p;
   delete p;

   // The buffer will be used for reading the incoming request
   // and for reading the requested file.  The buffer size was
   // chosen somewhat arbitrarily to be appropriate for file
   // i/o and much larger than any request we ever expect.

   char buffer[ 10240 ];
   int bytes;

   // Ignore null requests.
   if ( ( bytes = recv( ts, buffer, sizeof( buffer ) - 1, 0 ) ) > 0 )
      {
      // Null-terminate the request and print it.
      buffer[ bytes ] = 0;
      cout << endl << buffer;
       
      // Parse the request to find the action and path being requested.
      RequestHeader r = ParseRequestHeader( buffer, bytes );

      // Watch for a plugin that intercepts this path.
      if ( Plugin && Plugin->MagicPath( r.Path ) )
         {
         string request( buffer, bytes );
         string response = Plugin->ProcessRequest( request );
         cout << response << endl;
         send( ts, response.c_str( ), response.size( ), 0 );
         close( ts );
         return nullptr;
         }

      if ( r.Action == "GET" && r.Path != "" )
         {
         cout << "Requested path = " << r.Path << endl;

         if ( SafePath( r.Path.c_str( ) ) )
            {
            string completePath = RootDirectory + r.Path;
            cout << "Actual path = " << completePath << endl << endl;

            int f = open( completePath.c_str( ), O_RDONLY );

            if ( f != -1 )
               {
               // The path exists but it could be either a file
               // or a directory.

               off_t filesize = FileSize( f );
               if ( filesize != -1 )
                  {
                  // The is a file, not a directory.
                  string okMessage = "HTTP/1.1 200 OK\r\n"
                                      "Content-Length: ";
                  okMessage += to_string( filesize );
                  okMessage += "\r\nConnection: close\r\nContent-Type: ";
                  okMessage += Mimetype( completePath );
                  okMessage += "\r\n\r\n";

                  cout << okMessage;
                  send( ts, okMessage.c_str( ), okMessage.length( ), 0 );

                  while ( bytes = read( f, buffer, sizeof( buffer ) ) )
                     send( ts, buffer, bytes, 0 );
                  }
               else
                  {
                  // Attempts to read a directory are denied.
                  // (Might consider looking for the usual index.htm
                  // or similar files.)
                  cout << "Attempt to read a directory." << endl;
                  AccessDenied( ts );
                  }

               close( f );
               }
            else
               {
               // Path does not exist.
               cout << "Path does not exist." << endl;
               FileNotFound( ts );
               }
            }
         else
            {
            // Attempt to access something outside the website with too
            // many .. segments.
            cout << "Unsafe path blocked." << endl;
            AccessDenied( ts );
            }
         }
      else
         {
         // Wasn't an identifiable GET request.
         cout << "Garbled request." << endl;
         FileNotFound( ts ); 
         }
      }

   close( ts );
   return nullptr;
   }


void PrintAddress( const sockaddr_in *s, const size_t saLength )
   {
   const struct in_addr *ip = &s->sin_addr;
   uint32_t a = ntohl( ip->s_addr );

   // A better alternative here might be to use the inet_ntop( )
   // function, which can handle IPv6 addresses. But this does
   // show clearly how an ordinary IPv4 address is encoded.

   cout <<  ( a >> 24 ) << '.' <<
            ( ( a >> 16 ) & 0xff ) << '.' <<
            ( ( a >> 8 ) & 0xff ) << '.' <<
            ( a & 0xff ) << ":" <<
            ntohs( s->sin_port ) << endl;
   }


int main( int argc, char **argv )
   {
   if ( argc != 3 )
      {
      cerr << "Usage:  " << argv[ 0 ] << " port rootdirectory" << endl;
      return 1;
      }

   int port = atoi( argv[ 1 ] );
   RootDirectory = argv[ 2 ];

   // Discard any trailing slash.  (Any path specified in
   // an HTTP header will have to start with /.)

   char *r = RootDirectory;
   if ( *r )
      {
      do
         r++;
      while ( *r );
      r--;
      if ( *r == '/' )
         *r = 0;
      }

   // We'll use two sockets, one for listening for new
   // connection requests, the other for talking to each
   // new client.

   int listenSocket, talkSocket;

   // Create socket address structures to go with each
   // socket.

   struct sockaddr_in listenAddress,  talkAddress;
   socklen_t talkAddressLength = sizeof( talkAddress );
   memset( &listenAddress, 0, sizeof( listenAddress ) );
   memset( &talkAddress, 0, sizeof( talkAddress ) );
   
   // Fill in details of where we'll listen.
   
   // We'll use the standard internet family of protocols.
   listenAddress.sin_family = AF_INET;

   // htons( ) transforms the port number from host (our)
   // byte-ordering into network byte-ordering (which could
   // be different).
   listenAddress.sin_port = htons( port );

   // INADDR_ANY means we'll accept connections to any IP
   // assigned to this machine.
   listenAddress.sin_addr.s_addr = htonl( INADDR_ANY );

   // Create the listenSocket, specifying that we'll r/w
   // it as a stream of bytes using TCP/IP.

   listenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
   assert( listenSocket != -1 );

   // Bind the listen socket to the IP address and protocol
   // where we'd like to listen for connections.

   int bindResult = bind( listenSocket, ( sockaddr * )&listenAddress,
         sizeof( listenAddress ) );
   if ( bindResult )
      {
      // On WSL, the bind will fail if we're not running as sudo.
      cerr << "Bind failed, errno = " << errno << endl;
      cerr << "Try running with sudo." << endl;
      exit( errno );
      }

   // Begin listening for clients to connect to us.

   // The second argument to listen( ) specifies the maximum
   // number of connection requests that can be allowed to
   // stack up waiting for us to accept them before Linux
   // starts refusing or ignoring new ones.
   //
   // SOMAXCONN is a system-configured default maximum socket
   // queue length.  (Under WSL Ubuntu, it's defined as 128
   // in /usr/include/x86_64-linux-gnu/bits/socket.h.)

   int listenResult = listen( listenSocket, SOMAXCONN );
   assert( listenResult == 0 );

   cout << "Listening on ";
   PrintAddress( &listenAddress, sizeof( listenAddress ) );

   // Accept each new connection and create a thread to talk with
   // the client over the new talk socket that's created by Linux
   // when we accept the connection.

   while ( ( talkAddressLength = sizeof( talkAddress ),
         talkSocket = accept( listenSocket, ( sockaddr * )&talkAddress,
            &talkAddressLength ) ) && talkSocket != -1 )
      {
      cout << endl << "Connection accepted from ";
      PrintAddress( &talkAddress, talkAddressLength );

      // When creating a child thread, you get to pass a void *,
      // usually used as a pointer to an object with whatever
      // information the child needs.
      
      // The talk socket is passed on the heap rather than with a
      // pointer to the local variable because we're going to quickly
      // overwrite that local variable with the next accept( ).  Since
      // this is multithreaded, we can't predict whether the child will
      // run before we do that.  The child will be responsible for
      // freeing the resource.  We do not wait for the child thread
      // to complete.
      //
      // (A simpler alternative in this particular case would be to
      // caste the int talksocket to a void *, knowing that a void *
      // must be at least as large as the int.  But that would not
      // demonstrate what to do in the general case.)

      pthread_t child;
      pthread_create( &child, nullptr, Talk, new int( talkSocket ) );
      pthread_detach( child );
      }

   close( listenSocket );
   }
