//
// Copyright 2015 KISS Technologies GmbH, Switzerland
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Component: UTIL
//


#ifndef CPP_LIB_UTIL_H
#define CPP_LIB_UTIL_H

#include "cpp-lib/assert.h"
#include "cpp-lib/units.h"
#include "cpp-lib/sys/file.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/multi_array.hpp"
#include "boost/range/iterator_range.hpp"

#include <condition_variable>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <queue>
#include <vector>

#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cstring>
#include <cctype>


namespace cpl {

  
namespace detail_ {

//
// A standard auto_resource_traits class for resource type R.
//

template< typename R >
struct auto_resource_traits ;


} // namespace detail


namespace util {

//
// Return current Universal Time Coordinated [s] since a fixed epoch.
// Clock resolution is ~100Hz.
//
// TODO: The epoch is currently not guaranteed, but will typically be
// 00:00 on January 1, 1970.
//

double utc() ;


//
// Returns the struct tm in UTC based on the given UTC value [s] since
// 00:00 on January 1, 1970.
//

std::tm utc_tm(double utc);


//
// Returns the number of full days since January 1, 1970 for the given utc [s].
//
// Currently only defined for positive UTC values.
//

inline long day_number( double const utc ) {
  assert( utc >= 0 ) ;
  return static_cast< long >( utc / cpl::units::day() ) ;
}

inline long long llmilliseconds( double const t ) {
  return std::llrint(t * 1000.0);
}

inline const char* time_format_hh_mm() {
  return "%H:%M";
}

inline const char* time_format_hh_mm_ss() {
  return "%H:%M:%S";
}

//
// Returns a textual representation of the UTC date/time given in t
// ([s] since January 1, 1970).  The format is as for strftime(3).
// The default format results in an ISO 8601 combined date/time, e.g.:
//
//   2013-04-25T14:50:34Z
//
// Time is rounded to the nearest seconds.
//
// The function throws on errors.
//

std::string format_datetime(double t, const char* format = "%FT%TZ");

//
// Same as the above, but format date only, e.g.:
//
//   2013-04-25
//

inline std::string format_date( double const t )
{ return format_datetime(t, "%F"); }


//
// Same as the above, but format time only, e.g.:
//
//   14:50:34Z
//

inline std::string format_time( double const t )
{ return format_datetime(t, "%TZ"); }


//
// Formats dt [s] as [H:]MM[.t] (hours, minutes and
// tenths of minutes (second variant))
//
// If skip_hour is set, only print minutes if dt < 60.
//

std::string format_time_hh_mmt( double const& dt , bool skip_hour = true) ;
std::string format_time_hh_mm ( double const& dt , bool skip_hour = true) ;

//
// Same as the above, but no 'Z' at the end (it's still UTC)
//

inline std::string format_time_no_z( double const t )
{ return format_datetime(t, "%T"); }



//
// Parses a UTC date/time string in ISO8601 format and returns the 
// corresponding number of seconds since 00:00 on January 1, 1970.
//
// Example time string:
//
//   2013-04-25T14:50:34Z
//
// TODO: Use standardized functions!
//
// The function throws on parse and other errors.
//

double parse_datetime( 
    std::string const& , const char* const format = "%FT%TZ" ) ;

// Can be used with Boost string algorithms, e.g. split()
typedef boost::iterator_range<std::string::iterator> stringpiece;


//
// A timer which can be triggered and expires at a fixed time
// afterwards.
//
// Assumes monotonic time.
//

struct triggered_timer {

  // 
  // Set up a switch which will remain on for dt when triggered.
  //
 
  triggered_timer( double const& dt ) 
  : dt( dt )
  { assert( dt > 0 ) ; cancel() ; }

  //
  // Set the timer to untriggered state and start it.  
  // t is the current time.
  //

  void start( double const& t ) { t_trig = t + dt ; }


  //
  // Cancel the timer (set to untriggered state).
  //
  
  void cancel() { t_trig = std::numeric_limits< double >::max() ; }
  
  
  //
  // ``Manually'' trigger the timer (set to triggered state).
  //
  
  void trigger() { t_trig = -std::numeric_limits< double >::max() ; }
  
  
  //
  // Check if timer is triggered.  t is the current time.  Returns true
  // iff more that dt has elapsed since the last start() call or if
  // trigger() has been called.
  //

  bool triggered( double const& t ) const { return t >= t_trig ; }


private:

  double dt     ;
  double t_trig ;

} ;


//
// A simple scheduler for periodic actions requiring approximate timing.
//

struct simple_scheduler {

  //
  // Initialize with given time delta dt.
  //

  simple_scheduler( double const& dt = 0 ) ;

  //
  // Set time delta to dt.
  //
 
  void reconfigure( double const& dt ) ;

  //
  // Return true iff this is the first action() call, t is non-monotonic
  // (less than at the previous action() call) or more than dt has
  // elapsed since last action() call returning true.  The argument t is
  // the current time.
  //

  bool action( double const& t ) ;

private:

  // Time of last action() call returning true.
  double t_last ;

  double dt ;

} ;


// Check if argument is an integer and is in [ min , max ], throw an
// exception if not.

long check_long
( double const& x , double const& min , double const& max ) ;


/// \return Size of a built-in array (compile-time constant).

template< class T >
inline std::size_t size( T const& a )
{ return sizeof( a ) / sizeof( a[ 0 ] ) ; }


/// This tag can be used as a constructor argument to
/// signal that the constructor leaves fields uninitialized.
struct uninitialized {} ;


//
// Marks a value as being unused in order to avoid compiler warnings.
//

template< typename T >
void mark_unused( T const& t ) {
  static_cast< void >( t ) ;
}


//
// Pair operators
//

template< typename T1 , typename T2 > struct pair_less_first {
  
  bool operator()( 
    std::pair< T1 , T2 > const& p1 ,
    std::pair< T1 , T2 > const& p2
  ) const
  { return p1.first < p2.first ; }

} ;

template< typename T1 , typename T2 > struct pair_equal_first {
  
  bool operator()( 
    std::pair< T1 , T2 > const& p1 ,
    std::pair< T1 , T2 > const& p2
  ) const
  { return p1.first == p2.first ; }

} ;

template< typename T1 , typename T2 >
struct pair_access_first : std::unary_function< std::pair< T1 , T2 >& , T1& > {

  // 
  // operator() MUST be const.  This means that the object doesn't
  // change state.  (Easy if it doesn't have any :-)
  //

  T1& operator()( std::pair< T1 , T2 >& p ) const { return p.first ; }

} ;


//
// A safe getline() function with maximum number of characters.  Should
// be used if the input source is not trusted.
//
// CAUTION: Checks for '\n' only (not for '\r'); just as std::getline().
//
// Precondition: maxsize > 0
// Postcondition: s.size() <= maxsize
//
// A line longer than maxsize runs over into the next line.
//
// A line that is exactly maxsize long followed by '\n' causes
// an additional empty line to be read.
//
// size_hint can be used to tell the function to expect strings of
// approximately that size.
//

std::istream& getline(std::istream&, std::string& s, long maxsize,
    long size_hint = 0);

//
// Splits s on separator and inserts it into the given sequence.
//
// Usage e.g.:
//   std::vector< std::string > strs;
//   cpl::util::split("x,y,2", strs);
// Result: a vector containing "x", "y" and "2".
//

template< typename Sequence > void split( 
    Sequence& seq , std::string const& s , char const* const separators = "," ) {
  boost::algorithm::split(seq, s, boost::algorithm::is_any_of(separators));
}


//
// Copy is to os character-wise
//

inline void stream_copy( std::istream& is , std::ostream& os ) {

  std::copy(
    std::istreambuf_iterator< char >( is ) ,
    std::istreambuf_iterator< char >(    ) ,
    std::ostreambuf_iterator< char >( os )
  ) ;

}


//
// Writes a 2-dimensional boost multi_array to an ostream.
//

template<typename T>
std::ostream& write_array(std::ostream& os, boost::multi_array<T, 2> const& A) {

  for (unsigned long i = 0; i < A.shape()[0]; ++i) {
  for (unsigned long j = 0; j < A.shape()[1]; ++j) {
    if (!os) {
      return os;
    }

    os << A[i][j];

    if (j + 1 < A.shape()[1]) {
      os << ' ';
    }
  }
  os << '\n' ;
  }

  return os ;
}


//
// A per-thread death class.
//

struct death {

  death() : os( 0 ) {}

  death( std::ostream* os ) : os( os ) {}

  virtual ~death() { }

  void set_output( std::ostream* os_ ) { os = os_ ; }

  // Exit method may be overridden by the user.
  virtual void exit( int const code ) { std::exit( code ) ; }

  //
  // Try to write \a msg to os,
  // std::cerr, std::clog, \a name in turn.
  // Then call exit() with \a exit_code.  If \a name == "",
  // the system will try to write to "CPP_LIB_DIE_OUTPUT".
  //
  // os is given in the constructor or set by set_output().
  //

  void die(
    std::string const& msg ,
    std::string name = "" ,
    int const exit_code = 1
  ) ;

private:

  std::ostream* os ;

} ;


//
// File helpers
//

namespace file {

//
// Using
// http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// with a traits class to enable owning_istream/owning_ostream constructors
// with a buffer_maker<> argument that can be e.g. a network connection.
//
// The traits class is necessary because the derived class is considered
// incomplete upon deriving from the template:
// http://stackoverflow.com/questions/6006614/c-static-polymorphism-crtp-and-using-typedefs-from-derived-classes
//
// Usage example:
// struct connection : buffer_maker<connection> { ... } ;
// namespace file { 
//   template<> struct buffer_maker_traits<connection> { ... } ;
// }
//

// The traits class must define istreambuf_type and ostreambuf_type.
template< typename IO > struct buffer_maker_traits {} ;

// Type IO must define:
//   traits::istreambuf_type make_istreambuf() 
//   traits::ostreambuf_type make_ostreambuf() 
template< typename IO , typename traits = buffer_maker_traits< IO > >
struct buffer_maker {
  IO& downcast() { return *static_cast< IO* >( this ) ; }
  typename traits::istreambuf_type make_istreambuf() 
  { return downcast().make_istreambuf() ; }
  typename traits::ostreambuf_type make_ostreambuf() 
  { return downcast().make_ostreambuf() ; }
} ;

//
// owning_istream and owning_ostream are stream classes which
//
// 1. Always have a buffer.
// 2. Delete their current buffer on destruction.
//
// Notice that rdbuf() is a non-virtual function.  Do *not* call it
// on owning streams.
//
// Rationale: Is there any sense in having a stream without a buffer?
//
// These classes are a bit experimental as of 2014, in particular
// in view of slow adoption of move semantics for streams.
//

template< typename T >
struct owning_istream : std::istream {
  // Construct from buffer maker
  template< typename IO >
  owning_istream( buffer_maker< IO >& bm )
  : owning_istream( bm.make_istreambuf() ) {}

  // Construct from a buffer
  owning_istream( T&& buf )
  : std::istream( NULL ) , mybuf( std::move( buf ) )
  { rdbuf( &mybuf ) ; assert( rdbuf() ) ; }

  // Noncopyable
  owning_istream           ( owning_istream const& ) = delete;
  owning_istream& operator=( owning_istream const& ) = delete;

  // Moveable
  owning_istream( owning_istream&& other )
  : std::istream( std::move( other ) ) , mybuf( std::move( other.mybuf ) ) { 
    rdbuf( &mybuf ) ; 
  }

  owning_istream& operator=( owning_istream&& rhs ) {
    std::istream::operator=( std::move( rhs ) ) ;
    mybuf = std::move( rhs.mybuf ) ;
    rdbuf( &mybuf ) ;
    return *this;
  }

  T&       buffer()       { return mybuf; }
  T const& buffer() const { return mybuf; }

private:
  // This is mine!
  T mybuf ;

} ;


// Apparently, std::ostream *does* have a default constructor 
// for clang++-3.5 ?!  Using it resulted in disaster, however.
// So, initialize to NULL and immediately call rdbuf().
template< typename T >
struct owning_ostream : std::ostream {
  // Construct from buffer maker
  template< typename IO >
  owning_ostream( buffer_maker< IO >& bm )
  : owning_ostream( bm.make_ostreambuf() ) {}

  // Construct from buffer
  owning_ostream( T&& buf )
  : std::ostream( NULL ) , mybuf( std::move( buf ) )
  { rdbuf( &mybuf ) ; assert( rdbuf() ) ; }

  // Noncopyable
  owning_ostream           ( owning_ostream const& ) = delete ;
  owning_ostream& operator=( owning_ostream const& ) = delete ;

  // Moveable
  owning_ostream( owning_ostream&& other )
  : std::ostream( NULL ) , mybuf( std::move( other.mybuf ) ) 
  { rdbuf( &mybuf ) ; }

  owning_ostream& operator=( owning_ostream&& rhs ) {
    // http://www.cplusplus.com/reference/ostream/ostream/operator=/
    // 'same behaviour as calling member ostream::swap'
    // Does *not* exchange rdbuf() pointers.
    flush(); // is this needed?  
    std::ostream::operator=( std::move( rhs ) ) ;
    mybuf = std::move( rhs.mybuf ) ;
    rdbuf( &mybuf ) ;
    return *this;
  }

  T&       buffer()       { return mybuf; }
  T const& buffer() const { return mybuf; }

private:
  T mybuf ;

} ;

typedef owning_istream< std::filebuf > owning_ifstream;
typedef owning_ostream< std::filebuf > owning_ofstream;


//
// Open a file buffer for reading.
//
// Open the file.  If the operation fails, a std::runtime_error
// containing an appropriate message is thrown.
//
// The function tries all path names path[ i ] + name starting from
// i = 0 and finally tries name alone.
//
// After a successful open, which contains the full pathname of the
// opened file.
//

std::filebuf open_readbuf(
  std::string const& name  ,
  std::string      & which ,
  std::vector< std::string > const& path = std::vector< std::string >()
) ;


//
// Equivalent to the above function but without information about
// the opened file.
//

inline std::filebuf open_readbuf(
  std::string const& name ,
  std::vector< std::string > const& path = std::vector< std::string >()
) { std::string dummy ; return open_readbuf( name , dummy , path ) ; }


//
// Open a file by delegating to open_readbuf().
//

inline owning_ifstream open_read(
  std::string const& name  ,
  std::string      & which ,
  std::vector< std::string > const& path = std::vector< std::string >()
) 
{ return open_readbuf( name , which , path ) ; }


//
// Equivalent to the above function but without information about
// the opened file.
//

inline owning_ifstream open_read(
  std::string const& name ,
  std::vector< std::string > const& path = std::vector< std::string >()
) { std::string dummy ; return std::move( open_read( name , dummy , path ) ) ; }


// Open a file buffer for writing.

// Open the file.  If the operation fails, a std::runtime_error
// containing an appropriate message is thrown.

std::filebuf open_writebuf( 
    std::string const& name ,
    std::ios_base::openmode = std::ios_base::out | std::ios_base::binary ) ;


// Open a file for writing by delegating to open_writebuf().

inline owning_ofstream open_write( 
    std::string const& name ,
    std::ios_base::openmode const mode = 
        std::ios_base::out | std::ios_base::binary )
{ return open_writebuf( name , mode ) ; }


// Strip suffix from filename.

// If name ends in suffix, return name with suffix removed.
// Otherwise, return name.

std::string basename( std::string const& name , std::string const& suffix ) ;


// A class to redirect an existing ostream to a file.
// TODO: What's the use of this?

struct redirector {

  //
  // As long as the redirector exists, os is redirected to file name
  // by switching the buffer.
  //

  redirector( std::ostream& os , std::string const& name ) :
    file( cpl::util::file::open_write( name ) ) ,
    saved( os.rdbuf() ) ,
    redirected( os )
  { os.rdbuf( file.rdbuf() ) ; }

  /// Restore the original buffer.

  ~redirector() {
    always_assert( saved != 0 ) ;
    redirected.rdbuf( saved ) ;
  }

private:

  /// The file to redirect to.

  owning_ofstream file ;

  /// The saved stream buffer.

  std::streambuf* saved ;

  /// The redirected stream.

  std::ostream& redirected ;

} ;

//
// A file_name_queue stores up to n file names and begins
// deleting files at the end of the queue as soon as the
// limit is reached.
//
// Use case:  
// * Log file rotation
// * Temporary files which should exist for a while after being closed.
//

struct file_name_queue {

  //
  // Creates a file_name_queue handling at most n files.
  //

  file_name_queue( const long n )
  : maxsize( n ) {
    always_assert( maxsize > 0 ) ;
  }

  //
  // If size() == n, deletes the file at the tail of the
  // queue and removes its name from the queue.
  // Inserts filename at the head of the queue.
  // 
  // Returns true iff a file was deleted.
  //

  bool add( std::string const& filename ) ;

  long size() const { return q.size(); }

private:
  std::deque< std::string > q ;

  long maxsize ;

} ;

struct logfile_manager {

  //
  // Initializes a logfile_manager for at most n logfiles, the given
  // base name and current time.
  //
  // Files will be named basename.YYYY-MM-DD
  //

  logfile_manager(
      long n,
      std::string const& basename ,
      double utc_now ) ;

  //
  // Returns <basename>.YYYY-MM-DD (from utc).
  //

  std::string filename( double const utc ) const ;

  //
  // Update UTC.  If a new day has started, closes the current and
  // opens the next file in append mode.  Returns true iff a new file has been
  // opened.
  //

  bool update( double utc ) ;


  //
  // Returns the current file which is always open.
  //

  std::ostream& stream() { return os ; }

private:

  cpl::util::file::owning_ofstream newfile( double utc ) ;

  std::string basename ;
  long current_day ;
  cpl::util::file::file_name_queue q ;
  cpl::util::file::owning_ofstream os ;

} ;

template< typename T >
std::ostream& operator<<( cpl::util::file::logfile_manager& lfm, T const& t ) {
  return lfm.stream() << t ;
}

} // namespace file


//
// Set stream where die() should write its message.  \a os must
// point to a valid open ostream or be zero.
//

void set_die_output( std::ostream* os ) ;

//
// Construct a death object with the ostream given in set_die_output()
// and call its die() method with the given parameters.  Backward
// compatibility function, use of death class is preferred.
//

void die(
  std::string const& msg ,
  std::string name = "" ,
  int const exit_code = 1
) ;


//
// Toggle a boolean.
//

inline void toggle( bool& b ) { b = !b ; }


/// Lexical cast of arbitrary type to std::string.

template< typename T >
std::string string_cast( T const& ) ;

//
// Chop whitespace at the end of a string.
//

void chop( std::string& ) ;

//
// Tail (i.e., last n characters) of a string.
//

inline std::string tail(std::string const& s, std::size_t const n) {
  if (n >= s.size()) { return s;                      }
  else               { return s.substr(s.size() - n); }
}

//
// tolower/toupper for strings
//

inline void tolower(std::string& s)
{ for (char& c : s) { c = std::tolower(c); } }
inline void toupper(std::string& s)
{ for (char& c : s) { c = std::toupper(c); } }


//
// UTF-8 capable tolower/toupper
//
// Uses the en_US.UTF-8 locale.
//

std::string utf8_tolower(std::string const& s);
std::string utf8_toupper(std::string const& s);


// Verifies that a string contains only alphanumeric and
// possibly extra characters.  Throws cpl::util::value_error on violation.
void verify_alnum(std::string const& s, std::string const& extra = "");

// Writes a list of objects to os, optionally quoted by quote_char
// and with separators specified by sep.
template <typename IT> void
write_list(
    std::ostream& os, 
    IT it, IT const end,
    std::string const& sep = ",", 
    char const quote_char = 0) {

  if (end == it) {
    return;
  }

  while (true) {
    if (quote_char) { os << quote_char; }
    os << *it;
    if (quote_char) { os << quote_char; }

    ++it;
    if (it == end) {
      break;
    } else {
      os << sep;
    }
  }
}

//
// Writes a JSON style key-value pair
//
// Usage:
// std::cout << cpl::util::json("foo", 123);
// -> "foo": 123
// std::cout << cpl::util::json("hello", "world");
// -> "hello": "world"
//

template <typename T> struct json_wrapper {
  std::string const& key;
  T           const& value;
  json_wrapper(std::string const& k, T const& v)
  : key(k), value(v) {}
};

template<typename T>
inline json_wrapper<T> json(std::string const& key, T const& value) {
  return json_wrapper<T>(key, value);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, json_wrapper<T> const& j) {
  os << '"' << j.key << "\": ";
  if (   std::is_same<T, std::string>::value
      || std::is_same<T, const char*>::value) {
    os << '"';
  }
  os << j.value;
  if (   std::is_same<T, std::string>::value 
      || std::is_same<T, const char*>::value) {
    os << '"';
  }
  return os;
}

//
// Read characters from is until either is goes bad or the given
// character sequence string is encountered.
//

void scan_past( std::istream& is , char const* const string ) ;


//
// Converts an integer into a std::vector< char >.
//
// The most significant byte is put into the first element in the vector!
// The vector's size will be the same as type T's in byte.
//
// Author:  Marco Leuenberger.
//

template< typename T >
std::vector< char > to_char_vector( T const& ) ;


//
// Converts a std::vector< char > into an integer.
//
// The most significant byte must be the first element in the vector!
// The vector's size must be the same as type T's in byte!
//
// Author:  Marco Leuenberger.
//

template< typename T >
T to_integer( std::vector< char > const& ) ;


//
// Convert a (time) value t >= 0 into its integral part s and fractional
// part f, 0 <= f < m such that
//
//   t approximately equals s + f/m.
//
// S and F should be integral types.
//

template< typename S , typename F >
void to_fractional( double const& t , long const m , S& s , F& f ) {

  assert( m > 0 ) ;

  always_assert( t >= 0 ) ;

  // Round.
  double const tt = std::floor( t * m + .5 ) / m ;

  always_assert( tt >= 0 ) ;

  double const i = std::floor( tt ) ;

  s = static_cast< S >( i ) ;

  double const ff = m * ( tt - i ) ;
  always_assert( 0 <= ff       ) ;
  always_assert(      ff < m ) ;

  f = static_cast< F >( ff + .5 ) ;
  always_assert( 0 <= f     ) ;
  always_assert(      f < m ) ;

}


//
// A dirty hack to clear out an arbitrary structure.
//

template< typename T >
void clear( T& t ) {

  char* adr = reinterpret_cast< char* >( &t ) ;

  std::fill( adr , adr + sizeof( T ) , 0 ) ;

}

//
// A hopefully safe memcmp wrapper, compares byte-wise
//
// Returns: True iff both t1 and t2 are byte-wise equal.
//

template< typename T >
bool mem_equal( T const& t1 , T const& t2 ) {

  void const* const a1 = reinterpret_cast< void const* >( &t1 ) ;
  void const* const a2 = reinterpret_cast< void const* >( &t2 ) ;

  return 0 == memcmp( a1 , a2 , sizeof( T ) ) ;
}

//
// Unidirectional stream buffers based on the reader_writer concept.
//
// A reader_writer class holds a resource (e.g., file descriptor,
// socket, ...) and must expose two methods:
//
//   long read ( char      * const buf , long const n ) ;
//   long write( char const* const buf , long const n ) ;
//
// Both return either the number of characters read or written or -1
// on error.
//
// n is the number of bytes to be written or read and must be >= 1.
//
// read() tries to read at most n characters.  A return value of zero
// indicates end of file.
//
// write() tries to write n characters.  The return value is either n or
// -1.
//
// The buffer co-owns the reader_writer with the caller.
// This allows for multiple buffers to be used on a single resource, 
// e.g. an input and an output buffer on a network socket.
//
// This implementation is based on code by Nico Josuttis which was 
// accompanied by the following notice:
/*
 *
 * (C) Copyright Nicolai M. Josuttis 2001.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 *
 */

// 
// For implementation of streambufs, see also the newsgroup postings of
// Dietmar Kuehl, in particular:
//   http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&oe=UTF-8&selm=5b15f8fd.0309250235.3200cd43%40posting.google.com
//
// and
//
//   Josuttis, The C++ Standard Library, Chapter 13 (p. 678)
//
// The relevant standard chapter: 25.1.
//
// See also the crypt_istreambuf implementation.
//
// TODO: Implement xsputn().
// TODO: Maybe get rid of virtual inheritance.
//       In this case, the 'most derived' class initializes the virtual base,
//       causing significant confusion.
//       See http://stackoverflow.com/questions/27817478/virtual-inheritance-and-move-constructors
//

// From Nico's original comment:
/*
 * - open:
 *      - integrating BUFSIZ on some systems?
 *      - optimized reading of multiple characters
 *      - stream for reading AND writing
 *      - i18n
 */

template
< typename RW >
struct ostreambuf : std::streambuf {

  // TODO: Move assignment---also for istreambuf
  ostreambuf( ostreambuf const& ) = delete ;

  ostreambuf( ostreambuf&& other )
    : std::streambuf( static_cast<std::streambuf const&>( other ) ) ,
      rw    ( std::move( other.rw     ) ) ,
      buffer( std::move( other.buffer ) ) {

    // Important!! Invalidate other so that destructor becomes a NO-OP.
    // The destructor *is* being called after an object has been
    // moved from.
    other.setp(NULL, NULL);
  }

  ostreambuf& operator=(ostreambuf      &&) = delete;
  ostreambuf& operator=(ostreambuf const& ) = delete;

  //
  // Construct an ostreambuf on resource r with buffer size.
  //

  ostreambuf( std::shared_ptr<RW> const rw , int const size = 1024 )
  : rw    ( rw   ) ,
    buffer( size )
  { 
    
    always_assert( size >= 1 ) ; 
    setp( &buffer[ 0 ] , &buffer[ 0 ] + size - 1 ) ; 

  }

  ostreambuf( int const size = 1024 )
  : rw    ( NULL  ) ,
    buffer(  size ) {

    always_assert( size >= 1 ) ; 
    setp( &buffer[ 0 ] , &buffer[ 0 ] + size - 1 ) ; 

  }

  // Move constructor leaves rw (shared_ptr) empty.
  virtual ~ostreambuf() { 
    if (rw && pbase()) {
      sync() ; 
      rw->shutdown_write() ;
    }
  }

  void set_reader_writer( std::shared_ptr<RW> const newrw ) { 
    rw = newrw ;
  }

  RW& reader_writer() {
    assert( rw ) ;
    return *rw ;
  }

  RW const& reader_writer() const {
    assert( rw ) ;
    return *rw ;
  }

protected:

  //
  // See 27.5.2.4.5p3
  // Failure: EOF, success: != EOF.
  //

  virtual int_type overflow( int_type c ) {

    if( traits_type::eof() != c ) {

      *pptr() = traits_type::to_char_type( c ) ;
      pbump( 1 ) ;

    }

    return flush() ? c : traits_type::eof() ;

  }

  //
  // See 27.5.2.4.2p7. ``Returns -1 on failure''.
  //

  virtual int sync() { return flush() ? 0 : -1 ; }


private:

  //
  // Like sync(), but return a bool (true on success).
  //

  bool flush() ;

  std::shared_ptr<RW> rw ; 
  
  std::vector< char > buffer ;

} ;


template< typename RW >
struct istreambuf : virtual std::streambuf {

  istreambuf( istreambuf const& ) = delete ;

  istreambuf( istreambuf&& other )
    : std::streambuf( static_cast<std::streambuf const&>( other ) ) ,
      rw     ( std::move( other.rw      ) ) ,
      size_pb( std::move( other.size_pb ) ) ,
      buffer ( std::move( other.buffer  ) ) {

    // Important!! Invalidate other so that destructor becomes a NO-OP.
    // The destructor *is* being called after an object has been
    // moved from.
    other.setp(NULL, NULL);
  }

  istreambuf& operator=(istreambuf      &&) = delete;
  istreambuf& operator=(istreambuf const& ) = delete;

  
  //
  // Construct an istreambuf on resource r, buffer
  // size size, and putback area size size_pb.
  //

  istreambuf( 
    std::shared_ptr<RW> rw   , 
    int const size    = 1024 ,
    int const size_pb = 4 
  ) 
  : rw( rw ) ,
    size_pb( size_pb ) ,
    buffer( size + size_pb , 0 ) {

    always_assert( size    >= 1 ) ;
    always_assert( size_pb >= 1 ) ;
   
    char* const p = &buffer[ 0 ] + size_pb ; 
    setg( p , p , p ) ;

  }

  RW& reader_writer() {
    assert( rw ) ;
    return *rw ;
  }

  RW const& reader_writer() const {
    assert( rw ) ;
    return *rw ;
  }
  
  // Move constructor leaves rw (shared_ptr) empty.
  virtual ~istreambuf() 
  { if (rw) { rw->shutdown_read() ; } }

protected:

  virtual int_type underflow() ;

private:

  std::shared_ptr<RW> rw ;

  // Size of putback area.
  int const size_pb ; 

  std::vector< char > buffer ;

} ;


//
// A resource manager template with clean RAII and move semantics.
// It is designed to handle the typical Operating System specific
// handles like UNIX file descriptors, BSD sockets, Windows handles,
// etc.  The handle type is designated by 'R' below.
//
// It requires a traits class T with static methods
//
//   R    T::invalid()             -- Return an ``invalid'' resource 
//                                    handle (e.g., -1 for file handles).
//   bool T::valid( R const& h )   -- Return true iff handle h
//                                    is valid. 
//   void T::dispose( R const& h ) -- Return h to the OS.
//

template
< typename R , typename T = cpl::detail_::auto_resource_traits< R > >
struct auto_resource {

  auto_resource(                   ) : h( T::invalid()  ) {}
  auto_resource( R const h         ) : h( h             ) {}

  // Move
  auto_resource( auto_resource&& other ) : h( other.release() ) {}
  auto_resource& operator=( auto_resource&& rhs ) 
  { h = rhs.release() ; return *this ; }

  // Noncopyable
  auto_resource& operator=( auto_resource const& ) = delete ;
  auto_resource           ( auto_resource const& ) = delete ;

  R get() const { return h ; }

  bool valid() const { return T::valid( h ) ; }

  void reset( R const hh = T::invalid() ) { dispose() ; h = hh ; }

  ~auto_resource() { dispose() ; }

private:

  void dispose() const { if( T::valid( h ) ) { T::dispose( h ) ; } }

  R release() 
  { R const ret = h ; h = T::invalid() ; return ret ; }

  R h ;

} ;

////////////////////////////////////////////////////////////////////////
// Some simple thread-safe structures
////////////////////////////////////////////////////////////////////////

//
// A thread-safe queue that can have multiple writers and multiple
// readers.
//
// Heavily modified, based on a reply from
// http://stackoverflow.com/questions/15278343/c11-thread-safe-queue
// See also:
// http://en.cppreference.com/w/cpp/thread/condition_variable
//

template <class T> struct safe_queue {
  void push(T&& t) {
    {
      std::lock_guard<std::mutex> lock{m};
      q.push(std::move(t));
    }
    // "(the lock does not need to be held for notification)"
    c.notify_one();
  }

  T pop_front() {
    std::unique_lock<std::mutex> lock{m};

    // If q.empty(), this was a spurious wakeup
    // http://en.cppreference.com/w/cpp/thread/condition_variable/wait
    while (q.empty()) {
      c.wait(lock);
    }

    // lock is re-acquired after waiting, so we're good to go
    T const t = q.front();
    q.pop();
    return t;
  }

private:
  std::queue<T> q;
  std::mutex m;
  std::condition_variable c;
};


} // namespace util

} // namespace cpl


//
// Template definitions.
//

template< typename RW >
bool cpl::util::ostreambuf< RW >::flush() {

  if (!rw) { return true ; }

  long const n = pptr() - pbase() ;

  assert( 0 <= n                                         ) ;
  assert(      n <= static_cast< long >( buffer.size() ) ) ;

  if( 0 == n ) { return true ; }

  long const written = rw->write( pbase() , n ) ;

  if( written < 0 ) { return false ; }

  assert( n == written ) ;

  pbump( -n ) ;
  assert( pptr() == &buffer[ 0 ] ) ; 
  return true ;

}


template< typename RW >
typename cpl::util::istreambuf< RW >::int_type
cpl::util::istreambuf< RW >::underflow() {

  if( gptr() < egptr() ) 
  { return traits_type::to_int_type( *gptr() ) ; }

  int const n_pb = 
    std::min( 
      static_cast< long >( gptr() - eback() ) , 
      static_cast< long >( size_pb          ) 
    ) ;

  std::memmove( &buffer[ 0 ] + size_pb - n_pb , gptr() - n_pb , n_pb ) ;

  long const read = 
    rw->read( &buffer[ 0 ] + size_pb , buffer.size() - size_pb ) ;
  
  if( read <= 0 ) { return traits_type::eof() ; }

  setg(
    &buffer[ 0 ] + size_pb - n_pb , // beginning of putback area
    &buffer[ 0 ] + size_pb        , // read position
    &buffer[ 0 ] + size_pb + read   // end of buffer
  ) ;

  return traits_type::to_int_type( *gptr() ) ;

}

//
// Template definitions.
//

template< typename T >
std::string cpl::util::string_cast( T const& x ) {

  std::ostringstream os ;
  os << x ;
  return os.str() ;

}


template< typename T >
std::vector< char > cpl::util::to_char_vector( T const& x ) {

  assert( std::numeric_limits< T >::is_integer );

  std::vector< char > vec;
  unsigned int size;

  if( std::numeric_limits< T >::is_signed )
  {
    size = ( std::numeric_limits< T >::digits + 1 ) / 8 ;
  }
  else
  {
    size = std::numeric_limits< T >::digits / 8 ;
  }
     
  for( unsigned int i = size ; i > 0 ; --i )
  {
    vec.push_back( static_cast< char >( ( x >> ( ( i-1 ) * 8 ) ) & 0xFF ) );      
  }
  
  return vec ;

}


template< typename T >
T cpl::util::to_integer( std::vector< char > const& vec ) {

  assert( vec.size() == sizeof( T ) );

  T x = static_cast< T >( static_cast< unsigned char > ( vec[0] ) );
  
  for( unsigned int i = 1 ; i < vec.size() ; ++i )
  {
    x <<= 8;
    x = x + static_cast< T >( static_cast< unsigned char > ( vec[i] ) );
  }
  
  return x ;

}

#endif // CPP_LIB_UTIL_H
