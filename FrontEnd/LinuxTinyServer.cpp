// Linux tiny HTTP server.
// Nicole Hamilton  nham@umich.edu

// This variation of LinuxTinyServer supports a simple plugin interface
// to allow "magic paths" to be intercepted.  (But the autograder will
// not test this feature.)

// Usage:  LinuxTinyServer port rootdirectory

// Compile with g++ -pthread LinuxTinyServer.cpp -o LinuxTinyServer
// To run under WSL (Windows Subsystem for Linux), may have to elevate
// with sudo if the bind fails.

// LinuxTinyServer does not look for default index.htm or similar
// files.  If it receives a GET request on a directory, it will refuse
// it, returning an HTTP 403 error, access denied.

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
   // TO DO: if a matching a extentsion is found return the corresponding
   // MIME type.
   
   // Anything not matched is an "octet-stream", treated
   // as an unknown binary, which can be downloaded.
   size_t lastDotIdx = filename.find_last_of('.');

   if (lastDotIdx == string::npos) 
   {
      return "application/octet-stream";
   }
   const char *extension = (filename.substr(lastDotIdx)).c_str();
   //  std::cout << "extension: " << extension << std::endl;
   int NumberofTags = 66;
   int left = 0, right = NumberofTags - 1;
   while (left <= right) {
      int mid = (left + right) / 2;
      // Use std::tolower on the fly instead of modifying original string
      int compare = strcasecmp(MimeTable[mid].Extension, extension);
      if (compare == 0) {
         return MimeTable[mid].Mimetype;
      }
      if (compare < 0) {
         left = mid + 1;
      } else {
         right = mid - 1;
      }}
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
   // The path must start with a /.
   if ( *path != '/' )
      return false;

   // TO DO:  Return false for any path containing ..
   // segments that attempt to go higher than the root
   // directory for the website.
   const char *p = path;
   int level = 0;
   while(*p && *(p+1))
   {    
        if (strncmp(p, "../", 3) == 0)
        {
            if (--level < 0)
            {
                return false;
            }
            p += 3;
        }
      
      else
      {  
        while(*p && *p != '/')
        {
            ++p;
        }
        if (*p == '/')
        {
            ++level;
            ++p;
        }

      }
   }
   return true;


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


   void *Talk(void *talkSocket) {
      int *p = (int *)talkSocket;
      int ts = *p;
      delete p;
   
      std::string request;
      char buffer[4096];
      int totalBytes = 0;

      while (true) {
         int bytesRead = recv(ts, buffer, sizeof(buffer), 0);
         if (bytesRead <= 0) break;
         request.append(buffer, bytesRead);
         totalBytes += bytesRead;

         // Once we get headers, check Content-Length
         size_t headersEnd = request.find("\r\n\r\n");
         if (headersEnd != std::string::npos) {
            size_t bodyStart = headersEnd + 4;

            size_t cl = request.find("Content-Length:");
            if (cl != std::string::npos) {
                  size_t valueStart = cl + 15;
                  while (valueStart < request.size() && isspace(request[valueStart])) valueStart++;
                  size_t valueEnd = request.find("\r\n", valueStart);
                  int contentLength = std::stoi(request.substr(valueStart, valueEnd - valueStart));

                  if (request.size() >= bodyStart + contentLength) {
                     break; // Full body received
                  }
            } else {
                  break; // No Content-Length; assume single read
            }
         }
      }

   
      // Debug logging (optional)
      // std::cout << "Request received:\n" << request << std::endl;
   
      // Parse HTTP method
      size_t methodEnd = request.find(' ');
      if (methodEnd == std::string::npos) {
         close(ts);
         return nullptr;
      }
      std::string method = request.substr(0, methodEnd);
   
      // Parse path
      size_t pathStart = methodEnd + 1;
      size_t pathEnd = request.find(' ', pathStart);
      if (pathEnd == std::string::npos) {
         close(ts);
         return nullptr;
      }
      std::string path = request.substr(pathStart, pathEnd - pathStart);
      path = UnencodeUrlEncoding(path);
   
      // Plugin handles request (regardless of method)
      if (Plugin && Plugin->MagicPath(path)) {
         std::string response = Plugin->ProcessRequest(request);
         send(ts, response.c_str(), response.size(), 0);
         close(ts);
         return nullptr;
      }
   
      // Only GET methods should reach here
      if (method != "GET") {
         close(ts);
         return nullptr;
      }
   
      // Security check
      if (!SafePath(path.c_str())) {
         AccessDenied(ts);
         close(ts);
         return nullptr;
      }
   
      // Serve static file
      std::string absolutePath = RootDirectory + path;
      int fd = open(absolutePath.c_str(), O_RDONLY);
      if (fd == -1) {
         FileNotFound(ts);
      } else {
         off_t fileSize = FileSize(fd);
         const char *mimeType = Mimetype(path);
         std::string header = "HTTP/1.1 200 OK\r\n";
         header += "Content-Type: " + std::string(mimeType) + "\r\n";
         header += "Content-Length: " + std::to_string(fileSize) + "\r\n";
         header += "Connection: close\r\n\r\n";
         send(ts, header.c_str(), header.size(), 0);
   
         char fileBuffer[1024];
         ssize_t bytesRead;
         while ((bytesRead = read(fd, fileBuffer, sizeof(fileBuffer))) > 0) {
            send(ts, fileBuffer, bytesRead, 0);
         }
         close(fd);
      }
   
      close(ts);
      return nullptr;
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
   listenAddress.sin_port = htons( port ); // host to network short

   // INADDR_ANY means we'll accept connections to any IP
   // assigned to this machine.
   listenAddress.sin_addr.s_addr = htonl( INADDR_ANY ); // host to network long

   // TO DO:  Create the listenSocket, specifying that we'll r/w
   // it as a stream of bytes using TCP/IP.
   listenSocket = socket(AF_INET, SOCK_STREAM, 0);
   // TO DO:  Bind the listen socket to the IP address and protocol
   // where we'd like to listen for connections.
   socklen_t listenAddressLength = sizeof(listenAddress);

   if (::bind(listenSocket, (struct sockaddr*)&listenAddress, listenAddressLength) < 0) 
   {
      perror("bind failed");
      close(listenSocket);
      exit(EXIT_FAILURE);
   }

   if (listen(listenSocket, 128) < 0) {
      perror("listen");
      close(listenSocket);
      exit(EXIT_FAILURE);
   }
   // TO DO:  Begin listening for clients to connect to us.

   // The second argument to listen( ) specifies the maximum
   // number of connection requests that can be allowed to
   // stack up waiting for us to accept them before Linux
   // starts refusing or ignoring new ones.
   //
   // SOMAXCONN is a system-configured default maximum socket
   // queue length.  (Under WSL Ubuntu, it's defined as 128
   // in /usr/include/x86_64-linux-gnu/bits/socket.h.)

   // TO DO;  Accept each new connection and create a thread to talk with
   // the client over the new talk socket that's created by Linux
   // when we accept the connection.
   while( ( talkAddressLength = sizeof(talkAddress), talkSocket = accept(listenSocket, (sockaddr *)&talkAddress, &talkAddressLength)) &&talkSocket != -1)
      {

      // TO DO:  Create and detach a child thread to talk to the
      // client using pthread_create and pthread_detach.

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
      pthread_t thread;
      pthread_create(&thread, nullptr, Talk, new int (talkSocket));
      pthread_detach(thread);
      }

   close( listenSocket );
   }
