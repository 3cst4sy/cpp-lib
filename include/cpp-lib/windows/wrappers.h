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

//
// TODO:
// - Test after the streambuf_traits -> reader_writer refactor.
//

#ifndef CPP_LIB_WINDOWS_WRAPPERS
#define CPP_LIB_WINDOWS_WRAPPERS

#ifndef WIN32
  #error "Sorry: The feature requested is only available on Windows."
#endif

#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
// #include <wspiapi.h> // not on MinGW
// #include <windows.h>

#undef min
#undef max

#include <string>
#include <streambuf>
#include <vector>
#include <ios>

#include "cpp-lib/util.h"



//
// Socket stuff won't work unless both are defined...
//

#if defined( _WIN32 ) && !defined( WIN32 )
  #error "winsock2 needs both WIN32 and _WIN32 defined."
#endif


namespace cpl {


// 
// Entities not to be used directly by application code.
//

namespace detail_  {


// 
// See the corresponding function in posix_wrappers.h .  Under Windows,
// this is a NOOP because EINTR is not returned.  (TODO: verify!)
//

template< typename T > inline bool EINTR_repeat( T const ret ) 
{ return false ; }


//
// (U)LARGE_INTEGER conversion functions.  These types are undocumented. 
// This _might_ just work.
//

// 
// Round a double >= 0 to the nearest integer and return the equivalent
// LARGE_INTEGER.
//

LARGE_INTEGER const to_large_integer( double const& ) ;

//
// Convert a LARGE_INTEGER to an equivalent double.
//

inline double const to_double( LARGE_INTEGER const& i ) 
{ return i.u.HighPart * 4294967296. + i.u.LowPart ; }

//
// Convert an ULARGE_INTEGER to an equivalent double.
//

inline double const to_double( ULARGE_INTEGER const& i ) 
{ return i.u.HighPart * 4294967296. + i.u.LowPart ; }


//
// Return Windows ``performance counter'' frequency.
//

double const pc_frequency() ;


//
// Return current time since some epoch.  Based on
// QueryPerformanceCounter().
//

double const windows_time() ;


//
// Return the message given by Windows depending on GetLastError().
//

std::string const last_error_message() ;

 
//
// Throw a std::runtime_error containing \a msg followed by a colon and
// the result of last_error_message().
//

void last_error_exception( std::string const& msg ) ;


//
// An auto_resource_traits for Windoze (file) handles.
//

template<> struct auto_resource_traits< HANDLE > {

  static HANDLE const invalid() { return INVALID_HANDLE_VALUE ; }

  static bool valid  ( HANDLE const& h ) 
  { return invalid() != h && NULL != h ; }

  static void dispose( HANDLE const& h ) 
  { CloseHandle( h ) ; } 

} ;


// 
// auto_resource<> specialization for Windoze handles.
//

typedef cpl::util::auto_resource< HANDLE > auto_handle ;


//
// reader_writer implementation for Windoze ReadFile()/WriteFile().
//

struct windows_reader_writer {

  auto_handle fd;

  long read (char      * const buf , long const n ) ;
  long write(char const* const buf , long const n ) ;

  void shutdown_read () {}
  void shutdown_write() {}

} ;


//
// istreambuf<> and ostreambuf<> specializations for Windoze files.
//

typedef cpl::util::istreambuf< windows_reader_writer > windows_istreambuf ;
typedef cpl::util::ostreambuf< windows_reader_writer > windows_ostreambuf ;


//
// Open a file according to om and return its HANDLE.  om may be
// in, out or in|out.  File may shared by other open calls for reading.
// Throws on failure.
//

HANDLE windows_open
( std::string const& name , 
  std::ios_base::openmode const om = std::ios_base::in | std::ios_base::out ) ;


//
// Return last modification time [s] (since some fixed epoch) of file
// represented by fd.
//

double const modification_time( HANDLE const fd ) ;


//
// A struct wrapping a windows file.
//

struct file_impl {

  file_impl( std::string const& name ) 
  : fd( windows_open( name , std::ios_base::in ) ) {}

  // Return last access time [s] (since some fixed epoch).
  double const modification_time() 
  { return ::cpl::detail_::modification_time( fd.get() ) ; }

private:

  auto_handle fd ;

} ;


//
// Round a nonnegative time [s] to nearest nanosecond/microsecond and 
// return the respective timeval structure.
//
// Extensive checks including loss of precision.
//
::timeval  const to_timeval ( double const& t ) ;

//
// Convert a timeval to double.
//
inline double to_double( ::timeval  const& t )
{ return t.tv_sec + 1e-6 * t.tv_usec ; }


//
// The Windows tty implementation.
//

struct tty_impl {

  tty_impl( HANDLE h , int const n ) :
  fd( h ) , ibuf( NULL ) , obuf() 
  { always_assert( fd.valid() ) ; }

  auto_handle fd ;

  windows_istreambuf ibuf ;
  windows_ostreambuf obuf ;

} ;


//
// The Windoze socket type.
//

typedef SOCKET socketfd_t ;


//
// Winsock specific socket shutdown.
//

inline void socket_shutdown_read (const socketfd_t s) 
{ ::shutdown(s, SD_SEND   ); }
inline void socket_shutdown_write(const socketfd_t s)
{ ::shutdown(s, SD_RECEIVE); }

//
// Remove a file, returning unlink() error code.
//
inline int remove(std::string const& name)
{
	return ::_unlink(name.c_str());
}

// 
// Throw an exception of the form s + ": " + strerror( errnum ).
//
// If errnum is 0, use errno as errnum.
// 
// Standard, but NOT thread safe!
//
void strerror_exception(std::string const& s, int const errnum = 0);

} // namespace detail_

} // namespace cpl

#define timegm _mkgmtime

struct tm *gmtime_r(const time_t *timep, struct tm *result);

char* strptime(const char* buf, const char* fmt, struct tm* tm);

#endif // CPP_LIB_WINDOWS_WRAPPERS
