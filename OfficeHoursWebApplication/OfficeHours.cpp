#include <iostream>
#include <string>
using namespace std;

#include "List.h"
#include <ctype.h>
#include <cstdio>

#include "Plugin.h"
#include "Mutex.h"

// #define Debugging
#define LimitedDebugging

/*
#include "json.hpp"
using nlohmann::json;
*/

// Basic json grammar:

// <JsonObject>   ::= '{' [ <PropertyList> ] '}'
// <PropertyList> ::= <Property> { ',' <Property> }
// <Property>     ::= <Name> ':' <Value>
// <Name>         ::= <Token>
// <Value>        ::= <Array> | <JsonObject> | <Token>
// <Array>        ::= '[' <Value> {, <Value> } ']'

// A token is any one-character token ( { } , : [ ] ), a string
// surrounded by double quotes or any string of characters
// separated by white space or a one-character reserved token.

// <Token>        ::= <Reserved> | '"' [ ^"]* '"' |
//                      { [^ \t\r\n\v{},:\[\]]* } | <EOFString>

// <Reserved>     ::=  [{},:\[\]]

// Throws BadParse exceptions on malformed input.  (Does not
// use assert( ) to avoid crashing the server.)


// Possible simplification:

// Could simplify by insisting that there will always be whitespace
// around the tokens allowing string token << cin to be sufficient for
// tokenizing.
//
// Strings would be taken as a sequence of one or more cin tokens
// with a double quote at the beginning of the first and another
// double quote at the end of the last, collected as a single
// string with a single space character between the tokens.
// Would collapse multiple spaces to one but who cares.


const char EOFString[ ] = { EOF, 0 };

class BadParse
   {
   public:
      BadParse( )
         {
         }
      ~BadParse( )
         {
         }
   };

class Tokenizer
   {
   private:
      char c;  // c is the next char not yet consumed.
      bool lineBegin;
      string token;
      bool validToken;

      // Assume null-terminated.
      const char *source, *p, *endSource;

      char skipWhitespace( )
         {
         // Find the next non-whitespace character but stop
         // at blank lines.
         while ( c )
            {
            switch ( c )
               {
               // List of whitespace chars copied from isspace( ).

               case '\n':
                  if ( lineBegin )
                     // Stop at blank lines. 
                     return c;
                  lineBegin = true;

               case '\r':
                  // \r is not seen in the test input.
                  break;

               case ' ':
               case '\t':
               case '\v':
                  lineBegin = false;
                  break;

               default:
                  lineBegin = false;
                  return c;
               }

            c = *++p;
            }
         return c;
         }

      bool OneCharacterToken( char c )
         {
         switch ( c )
            {
            case ':':
            case '{':
            case '}':
            case '[':
            case ']':
            case ',':
               return true;
            }
         return false;
         }

      void collectToken( )
         {
         token = "";
         if ( OneCharacterToken( c ) )
            {
            token += c;
            c = *++p;
            }
         else
            switch ( c )
               {
               case '\n':
                  if ( !lineBegin )
                     throw BadParse( );
                  // Special case of a blank line.
                  // Consume the character and return as a null token.
                  // set c = ' ' to allow skipping next time.
                  c = ' ';
                  break;

               case 0:
                  // End of source, do not consume.
                  token = EOFString;
                  break;

               case '"':
                  // Collect until matching quote.
                  do
                     {
                     token += c;
                     c = *++p;
                     }
                  while ( c && c != '"' );
                  if ( c == '"' )
                     {
                     token += c;
                     c = *++p;
                     }
                  break;

               default:
                  // Collect anything until end of string, whitespace or a
                  // single-character token encountered.
                  do
                     {
                     token += c;
                     c = *++p;
                     }
                  while ( c && !isspace( c ) && !OneCharacterToken( c ) );
                  break;
               }
         }

   public:
      const string GetToken( )
         {
         if ( !validToken )
            {
            if ( c )
               {
               skipWhitespace( );
               collectToken( );
#              ifdef Debugging
                  cerr << "token = " << token << endl;
#              endif
               }
            else
               token = EOFString;
            validToken = true;
            }
         return token;
         }

      void AcceptToken( )
         {
         if ( token != EOFString )
            validToken = false;
         }

      bool Match( const char *token )
         {
         if ( GetToken( ) == token )
            {
#           ifdef Debugging
               cerr << "Matching " << token << endl;
#           endif
            AcceptToken( );
            return true;
            }
         return false;
         }

      void MatchAny( )
         {
         ( void )GetToken( );
         AcceptToken( );
         }

      const string GetRestOfLine( )
         {
         if ( !validToken )
            {
            if ( c )
               {
               skipWhitespace( );
               token = c;
               while ( ( c = *++p ) && c != '\r' && c != '\n' )
                  {
#                 ifdef Debugging
                     cerr << "c = " << c << endl;
#                 endif
                  token += c;
                  }
               lineBegin = false;
#              ifdef Debugging
                  cerr << "Rest of line token = " << token << endl;
#              endif
               }
            else
               token = EOFString;
            validToken = true;
            }
         return token;
         }

      void SkipBytes( size_t count )
         {
         p += count;
         if ( p > endSource )
            p = endSource;
         }

      void AssertMatch( const char *token )
         {
         bool m = Match( token );
         if ( !m )
            throw BadParse( );
         }

      Tokenizer( const string &src ) : lineBegin( false ), validToken( false ),
            source( src.c_str( ) ), p( src.c_str( ) ),
            endSource( src.c_str( ) + src.size( ) ),
            c( src[ 0 ] )
         {
         }

      ~Tokenizer( )
         {
         }
   };

const int IndentAmount = 4;

class ValueObj
   {
   public:
      virtual string StringView( int indentLevel ) = 0;
      virtual ValueObj *Copy( ) const = 0;
      virtual ~ValueObj( )
         {
         }
      static ValueObj *Find( Tokenizer &t );
   };


class StringValue : public ValueObj
   {
   public:
      string Value;
      string StringView( int indentLevel )
         {
         // Never indented.
         return Value;
         }
      StringValue *Copy( ) const
         {
         return new StringValue( Value );
         }
      StringValue( string value ) : Value( value )
         {
         }
      StringValue( )
         {
         }
      ~StringValue( )
         {
         }
   };


class Property : public ValueObj
   {
   public:
      string Name;
      ValueObj *Value;

      string StringView( int indentLevel )
         {
         string s = string( indentLevel*IndentAmount, ' ' ) + Name;
         s += ": ";
         s += Value->StringView( indentLevel + 1 );
         return s;
         }

      Property *Copy( ) const
         {
         return new Property( Name, Value->Copy( ) );
         }

      static Property *Find( Tokenizer &t )
         {
         string name = t.GetToken( );
         t.AcceptToken( );
         t.AssertMatch( ":" );
         ValueObj *value = ValueObj::Find( t );
         return new Property( name, value );
         }

      Property( string name, string value ) :
            Name( name ), Value( new StringValue( value ) )
         {
         }

      Property( string name, int value ) :
            Name( name )
         {
         char valueString[ 33 ];
         snprintf( valueString, sizeof( valueString ), "%d", value );
         Value = new StringValue( valueString );
         }

      Property( string name, ValueObj *value ) :
            Name( name ), Value( value )
         {
         }

      Property( ) : Value( nullptr )
         {
         }

      ~Property( )
         {
         delete Value;
         }
   };


class JsonObject : public ValueObj
   {
   public:
      // Properties are kept in sorted order.
      List< Property * > Properties;

      string StringView( int indentLevel )
         {
         string s = string( indentLevel*IndentAmount, ' ' ) + "{\r\n";
         bool comma = false;
         for ( auto p : Properties )
            {
            if ( comma )
               s += ",\r\n";
            comma = true;
            s += p->StringView( indentLevel + 1 );
            }
         s += "\r\n" + string( indentLevel*IndentAmount, ' ' ) + "}";
         return s;
         }

      JsonObject *Copy( ) const
         {
         JsonObject *j = new JsonObject( );
         for ( auto p : Properties )
            {
            Property *copy = p->Copy( );
            j->Properties.push_back( copy );
            }
         return j;
         }

      static JsonObject *Find( Tokenizer &t )
         {
         // A Json object consists of a list of properties
         // beginning with the opening { and ending with the
         // closing }.

         if ( t.Match( "{" ) )
            {
            JsonObject *j = new JsonObject( );

            // Discard any existing definition.
            Property *p;
            string token;

            do
               {
               p = Property::Find( t );
               j->Set( *p );
               delete p;
               }
            while ( t.Match( "," ) );

            t.AssertMatch( "}" );
            return j;
            }
         return nullptr;
         }

      void Set( const Property &property )
         {
         for ( auto i = Properties.begin( );  i != Properties.end( );  ++i )
            {
            Property *p = *i;
            if ( p->Name == property.Name )
               {
               // Exists, overwrite the value.
               delete p->Value;
               p->Value = property.Value->Copy( );
               return;
               }
            else
               if ( p->Name > property.Name )
                  {
                  // Should go here, before this property.
                  Properties.insert( i, property.Copy( ) );
                  return;
                  }
            }

         // Not found and goes after the last property.
         Properties.push_back( property.Copy( ) );
         }

      Property *Get( const string &propertyName )
         {
         for ( auto p : Properties )
            if ( p->Name == propertyName )
               return p;
         return nullptr;
         }

      JsonObject( )
         {
         }

      ~JsonObject( )
         {
         for ( auto p : Properties )
            delete p;
         }
   };


class ArrayValue : public ValueObj
   {
   public:
      List< ValueObj * > Objects;

      string StringView( int indentLevel )
         {
         string s = "[\r\n";
         for ( auto i : Objects )
            s += string( indentLevel*IndentAmount, ' ' ) + 
               i->StringView( indentLevel + 1 );
         s += "\r\n]";
         return s;
         }

      ArrayValue *Copy( ) const
         {
         ArrayValue *av = new ArrayValue( );
         for ( auto obj : Objects )
            av->Objects.push_back( obj->Copy( ) );
         return av;
         }

      static ArrayValue *Find( Tokenizer &t )
         {
         // It's a comma-separated list of values beginning with the
         // opening [ and ending with a closing ].

         if ( t.Match( "[" ) )
            {
            ArrayValue *av = new ArrayValue;

            while ( t.GetToken( ) != "]" )
               {
               av->Objects.push_back( ValueObj::Find( t ) );
               t.Match( "," );
               }

            t.AssertMatch( "]" );
            return av;
            }
         return nullptr;
         }
         
      ArrayValue( )
         {
         }

      ~ArrayValue( )
         {
         for ( auto obj : Objects )
            delete obj;
         }
   };


ValueObj *ValueObj::Find( Tokenizer &t )
   {
   // A value can be a simple token (or a string), an array of values,
   // or a Json object.

   ValueObj *v;

   if ( ( v = ArrayValue::Find( t ) ) )
      return v;

   if ( ( v = JsonObject::Find( t ) ) )
      return v;

   // Must be an ordinary string value.

   StringValue *s = new StringValue( t.GetToken( ) );
   t.AcceptToken( );
   return s;
   }


class OfficeHoursQueue
   {
   private:
      // List of json objects representing the office hours queue.

      List< JsonObject * > Queue;

   public:

      JsonObject *FindRequestHeader( Tokenizer &t )
         {
         // Parse the request header and return it as a json object.

         while ( t.Match( "" ) ) // Skip any blank lines.
            ;

         string request = t.GetToken( );
         t.AcceptToken( );
         string path = t.GetToken( );
         t.AcceptToken( );

         if ( request != EOFString && path != EOFString )
            {
            JsonObject *j = new JsonObject( );

            Property p;
            p.Name = "request";
            p.Value = new StringValue( request );
            j->Set( p );

            p.Name = "path";
            delete p.Value;
            p.Value = new StringValue( path );
            j->Set( p );

            t.GetRestOfLine( );
            t.AcceptToken( );

            // Collect the remainder of the header as a list of json
            // properties terminated by a blank line.

            string token;

#           ifdef Debugging
               cerr << "request = " << request << ", path = " << path << endl;
#           endif

            while ( ( token = t.GetToken( ) ) != "" && token != EOFString )
               {
               p.Name = t.GetToken( );
               t.AcceptToken( );
               t.AssertMatch( ":" );
               delete p.Value;
               p.Value = new StringValue( t.GetRestOfLine( ) );
               t.AcceptToken( );
#              ifdef Debugging
                  cerr << "new property = " << p.StringView( 0 ) << endl;
#              endif

               // If a property has already been set, this trivially overwrites
               // the old value.  Actual HTTP standard says order matters and
               // that the values should treated as identical to a comma-separated
               // list of those values.
               j->Set( p );

#              ifdef ContentLengthHack
                  if ( p.Name == "Content-Length" )
                     // This is a hack to get around a bug in server.py,
                     // which doesn't send the final blank line to mark the
                     // end of a header if there's no content.
                     break;
#              endif
               }

#           ifdef LimitedDebugging
               cerr << "Found request header:" << endl;
               cerr << j->StringView( 0 ) << endl;
#           endif

            return j;
            }
         else
            {
#           ifdef LimitedDebugging
               cerr << "No request header found" << endl;
#           endif
            return nullptr;
            }
         }

      string ResponseHeader( int code, const char *message, int length )
         {
         // HTTP/1.1 <code> <message>
         // Content-Type: application/json; charset=utf-8
         // Connection: close
         // Content-Length: <length>
         // \n

         string result = "HTTP/1.1 ";

         char valueString[ 33 ];
         snprintf( valueString, sizeof( valueString ), "%d", code );
         result += ( char * )valueString;

         result += " ";
         result += message;
         result += "\r\n"
                  "Content-Type: application/json; charset=utf-8\r\n"
                  "Connection: close\r\n"
                  "Content-Length: ";

         snprintf( valueString, sizeof( valueString ), "%d", length );
         result += ( char * )valueString;

         result += "\r\n\r\n";

#        ifdef Debugging
            cerr << "Response header:\n" << result;
#        endif
         return result;
         }

      string GetApi( )
         {
         // Response is always the same:
         // HTTP/1.1 200 OK
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 159
         // \n
         // {
         //     "queue_head_url": "http://localhost/queue/head/",
         //     "queue_list_url": "http://localhost/queue/",
         //     "queue_tail_url": "http://localhost/queue/tail/"
         // }
         // \n

         string indent( IndentAmount, ' ' );

         string result = "{\r\n";
         result += indent + "\"queue_head_url\": \"http://localhost/queue/head/\",\r\n";
         result += indent + "\"queue_list_url\": \"http://localhost/queue/\",\r\n";
         result += indent + "\"queue_tail_url\": \"http://localhost/queue/tail/\"\r\n";
         result += "}\r\n";
         result = ResponseHeader( 200, "OK", result.size( ) ) + result;
         return result;
         }

      string GetApiQueueHead( )
         {

         // Example response:
         // HTTP/1.1 200 OK
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 76
         // \n
         // {
         //     "location": "Table 3",
         //     "position": 1,
         //     "uniqname": "awdeorio"
         // }
         //
         // If queue is empty, return 400 error.

         if ( Queue.empty( ) )
            return Error( );
         else
            {
            string results = ( *Queue.front( ) ).StringView( 0 );
            results = ResponseHeader( 200, "OK", results.length( ) ) + results;
#           ifdef LimitedDebugging
               cerr << "Responding:\n" << results << endl;
#           endif
            return results;
            }
         }

      string GetApiQueue( )
         {
         // Example response:
         // HTTP/1.1 200 OK
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 411
         // \n
         // {
         //     "count": 3,
         //     "results": [
         //         {
         //             "location": "Table 3",
         //             "position": 1,
         //             "uniqname": "awdeorio"
         //         },
         //         {
         //             "location": "Table 15",
         //             "position": 2,
         //             "uniqname": "akamil"
         //         },
         //         {
         //             "location": "Desks behind bookshelves",
         //             "position": 3,
         //             "uniqname": "jklooste"
         //         }
         //     ]
         // }

         // If the queue is empty, the response should be:
         // HTTP/1.1 200 OK
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 39
         // \n
         // {
         //     "count": 0,
         //     "results": null
         // }

         string results;
         int position = 0;
         for ( auto j : Queue )
            {
            if ( position++ )
               results += ",\n";            
            results += ( *j ).StringView( 2 );
            }

         if ( position )
            {
            char countString[ 33 ];
            snprintf( countString, sizeof( countString ), "%d", position );

            string wrapper = "{\r\n";
            wrapper += string( IndentAmount, ' ' ) + "\"count\": ";
            wrapper += countString;
            wrapper += ",\r\n";
            wrapper += string( IndentAmount, ' ' ) + "\"results\": [\r\n";
            results = wrapper + results;
            results += "\r\n";
            results += string( IndentAmount, ' ' ) + "]\r\n}";
            }
         else
            {
            results = "{\r\n";
            results += string( IndentAmount, ' ' ) + "\"count\": 0,\r\n";
            results += string( IndentAmount, ' ' ) + "\"results\": null\r\n}";
            }

         results = ResponseHeader( 200, "OK", results.length( ) ) + results;
#        ifdef Debugging
            cerr << "Responding:\n" << results << endl;
#        endif
         return results;
         }

      string PostApiTail( Tokenizer &t )
         {
         // Example request:
         // POST /api/queue/tail/ HTTP/1.1
         // Host: localhost
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 57
         // \n
         // {
         //     "uniqname": "jackgood",
         //     "location": "Table 5"
         // }
         // \n
         
         // Find the Json object to be added.

         t.Match( "" ); // Gobble any blank line after the header.
         JsonObject *j = JsonObject::Find( t );

         // Add a property with its position in the queue.

         Property p( "\"position\"", Queue.size( ) + 1 );
         j->Set( p );
         
         // Add it to the queue.
         Queue.push_back( j );

         // Example response:
         // HTTP/1.1 201 Created
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 76
         // \n
         // {
         //     "location": "Table 5",
         //     "position": 1,
         //     "uniqname": "jackgood"
         // }

         string results = j->StringView( 0 );
         results = ResponseHeader( 201, "Created", results.length( ) ) + results;
#        ifdef Debugging
            cerr << "Responding:\n" << results << endl;
#        endif
         return results;
         }

      string DeleteApiQueueHead( )
         {
         // Response:
         // HTTP/1.1 204 No Content
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 0
         // \n
         
         // If the queue is empty, return a 400 error.
         if ( Queue.size( ) == 0 )
            return Error( );

         // Delete the first item.
         JsonObject *j = Queue.front( );
         Queue.pop_front( );
         delete j;

         // Renumber the rest.
         int position = 1;
         for ( auto j : Queue )
            {
            Property number( "\"position\"", position++ );
            j->Set( number );
            }

         return ResponseHeader( 204, "No Content", 0 );
         }

      string Error( )
         {
         // Response:
         // HTTP/1.1 400 Bad Request
         // Content-Type: application/json; charset=utf-8
         // Content-Length: 0
         // \n
         
         return ResponseHeader( 400, "Bad Request", 0 );
         }

      string RestServer( const string request )
         {
         // Must support adding reporting the queue, and adding to the end
         // and popping from the top.

         // Read REST API requests from the request string and returns
         // a result string that should be written to the client.

         // Exit on the first error.

         // Requests are of the form:
         // <request> <path> HTTP/1.1
         // Host: localhost
         // Content-Type: application/json; charset=utf-8
         // Content-Length: <length>
         // \n

         string result;
         try
            {
            Tokenizer t( request );

            if ( t.GetToken( ) != EOFString )
               {
   #           ifdef Debugging
                  cerr << "Looking for a new header." << endl;
   #           endif
               JsonObject *requestHeader = FindRequestHeader( t );

               if ( requestHeader )
                  {
                  Property *requestProperty = requestHeader->Get( "request" );
                  string request = requestProperty ?
                     requestProperty->Value->StringView( 0 ) : "";

                  Property *pathProperty = requestHeader->Get( "path" );
                  string path = pathProperty ?
                     pathProperty->Value->StringView( 0 ) : "";

                  if ( request == "GET" )
                     if ( path == "/api/" )
                        result = GetApi( );
                     else
                        if ( path == "/api/queue/head/" )
                           result = GetApiQueueHead( );
                        else               
                           if ( path == "/api/queue/" )
                              result = GetApiQueue( );
                           else
                              result = Error( );
                  else
                     if ( request == "POST" || request == "PUT" )
                        if ( path == "/api/queue/tail/" )
                           result = PostApiTail( t );
                        else
                           result = Error( );
                     else
                        if ( request == "DELETE" && path == "/api/queue/head/" )
                           result = DeleteApiQueueHead( );
                        else
                           result = Error( );

                  delete requestHeader;
                  }
               }
            }
         catch( ... )
            {
            result = Error( );
            }

         return result;
         }

      OfficeHoursQueue( )
         {
         }

      ~OfficeHoursQueue( )
         {
         for ( auto entry : Queue )
            delete entry;
         }
   };


class P4_Web : public PluginObject
   {
   private:
      Mutex lock;
      const string magicPath = "/api/";
      OfficeHoursQueue OH;

   public:
      bool MagicPath( const string path )
         {
         bool compare = path.substr( 0, magicPath.size( ) ) == magicPath;
         cerr << "comparing " << path << " and " << magicPath << " result = " << compare << endl;
         return compare;
         }

      string ProcessRequest( const string request )
         {
         cerr << "Processing:" << endl << request << endl;
         lock.Lock( );
         string result = OH.RestServer( request );
         lock.Unlock( );
         return result;
         }

      P4_Web( )
         {
         Plugin = this;
         }
      ~P4_Web( )
         {
         }
   };


P4_Web p4_web;