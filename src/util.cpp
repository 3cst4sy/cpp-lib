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

#include "cpp-lib/util.h"

#include <chrono>
#include <codecvt>
#include <iostream>
#include <fstream>
#include <memory>
#include <exception>
#include <stdexcept>
#include <limits>
#include <locale>

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "cpp-lib/exception.h"
#include "cpp-lib/platform/wrappers.h"

#include "boost/system/error_code.hpp"
#include "boost/locale.hpp"

using namespace cpl::util ;


namespace {

std::ostream* die_output = 0 ;

std::string const die_output_name() { return "CPP_LIB_DIE_OUTPUT" ; }

// Locale to use for UTF-8 stuff.
boost::locale::generator gen;
std::locale const utf8_locale = gen("en_US.UTF-8");

// UTF-8 <-> std::wstring conversion
// We do not want wstring in public interfaces, but 
// internally it is needed for operations like toupper, tolower.
std::wstring to_wstring(std::string const& s) {
  // Converters do have state.
  std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
  return conv.from_bytes(s);
}

std::string to_string(std::wstring const& s) {
  std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
  return conv.to_bytes(s);
}

} // anonymous namespace


void cpl::util::set_die_output( std::ostream* os )
{ die_output = os ; }

// TODO: Templatize, etc?
// http://en.cppreference.com/w/cpp/string/basic_string/getline
std::istream& cpl::util::getline(
    std::istream& is , std::string& s , 
    long const maxsize , long const size_hint ) {

  always_assert( maxsize > 0 ) ;

  // Do *not* skip whitespace!!
  std::istream::sentry sen( is , true ) ;
  if ( !sen ) {
    return is ;
  }

  s.clear() ;
  s.reserve( size_hint ) ;

  std::istreambuf_iterator< char > it( is ) ;
  std::istreambuf_iterator< char > const eos ;

  long i = 0 ;

  while( i < maxsize ) {
    if ( eos == it ) {
      is.setstate( std::ios_base::failbit | std::ios_base::eofbit ) ;
      return is ;
    }
    char const c = *it ;
    ++it ;
    if( '\n' == c ) {
      return is ;
    } else {
      s.push_back( c ) ;
      ++i ;
    }
  }

  return is ;
}


void cpl::util::death::die(
  std::string const& msg ,
  std::string name ,
  int const exit_code
) {

  if( os && *os        << msg << std::endl ) { goto suicide ; }
  if(       std::cerr  << msg << std::endl ) { goto suicide ; }
  if(       std::clog  << msg << std::endl ) { goto suicide ; }

  if( name == "" )
  { name = die_output_name() ; }

  {
    // If the following fails, we're really f**cked up :-)
    std::ofstream last_chance( name.c_str() ) ;
    last_chance << msg << std::endl ;
  }

suicide:
  this->exit( exit_code ) ;

}

void cpl::util::die(
  std::string const& msg ,
  std::string name ,
  int const exit_code
)
{ death( die_output ).die( msg , name , exit_code ) ; }


void cpl::util::assertion(
  const bool expr ,
  std::string const& expr_string ,
  std::string const& file ,
  const long line
) {

  if( expr ) return ;

  std::ostringstream os ;
  os << "Assertion failed: "
     << expr_string << " (" << file << ":" << line << ")" ;

  die( os.str() ) ;

}

void cpl::util::verify(
  const bool expression ,
  std::string const& message
) { 
  if( !expression ) { throw std::runtime_error( message ) ; }
}


long cpl::util::check_long
( double const& x , double const& min , double const& max ) {

  if( x < min || x > max ) {

    std::ostringstream os ;
    os << "should be between " << min << " and " << max ;
    throw std::runtime_error( os.str() ) ;

  }

  if( static_cast< long >( x ) != x )
  { throw std::runtime_error( "should be an integer" ) ; }

  return static_cast< long >( x ) ;

}


void cpl::util::scan_past( std::istream& is , char const* const s ) {

  char c ;

restart:

  char const* p = s ;

  while( *p && is.get( c ) ) {

    if( *p != c ) { goto restart ; }
    ++p ;

  }

}

double cpl::util::utc() {
  return 
    std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::tm cpl::util::utc_tm(double const t) {
  std::time_t const t1 = t + 0.5;
  std::tm ret;
  // Better safe than sorry: Set members to 0
  cpl::util::clear(ret);
  if (NULL == ::gmtime_r(&t1, &ret)) {
    cpl::detail_::strerror_exception("gmtime()", errno);
  }
  return ret;
}

std::string cpl::util::format_datetime(
    double const t, char const* const format) {
  std::tm t2 = cpl::util::utc_tm(t);

  char ret[100];
  if (0 == strftime(ret, 99, format, &t2)) {
    throw std::runtime_error("format_datetime(): formatting failure");
  }
  return ret;
}

std::string cpl::util::format_time_hh_mm(double const& dt,
                                         bool const skip_hour) {
  assert( dt >= 0 ) ;

  double h = std::floor(dt / cpl::units::hour());
  double m = std::round((dt - h * cpl::units::hour()) / cpl::units::minute());
  if (m >= 59.99) {
    m = 0;
    ++h;
  }

  char ret[30];
  if (h < 0.1 && skip_hour) {
    sprintf(ret, "%02.0lf", m);
  } else {
    sprintf(ret, "%.0lf:%02.0lf", h, m);
  }
  return ret;
}


std::string cpl::util::format_time_hh_mmt(double const& dt,
                                          bool const skip_hour) {
    
  assert( dt >= 0 ) ;

  double h = std::floor(dt / cpl::units::hour());
  // Round to nearest 10th of minute
  double m = 0.1 * std::round(
      10 * (dt - h * cpl::units::hour()) / cpl::units::minute());
  if (m >= 59.99) {
    m = 0;
    ++h;
  }

  char ret[30];
  if (h < 0.1 && skip_hour) {
    sprintf(ret, "%04.1lf", m);
  } else {
    sprintf(ret, "%.0lf:%04.1lf", h, m);
  }
  return ret;
}

double cpl::util::parse_datetime(
    std::string const& s, char const* format) {
  std::tm t2;

  // Set members to 0.  Absolutely necessary here since strptime()
  // may leave fields untouched, depending on the format.
  cpl::util::clear(t2);

  const char* const buf = s.c_str();
  const char* const res = strptime(buf, format, &t2);

  if (NULL == res) {
    throw std::runtime_error("parse_datetime(): parsing failure");
  }

  if (buf + s.size() != res) {
    throw std::runtime_error("parse_datetime(): parsing incomplete");
  }

  // from man mktime(3):
  // The timegm() function is not specified by any standard; its function can-
  // not be completely emulated using the standard functions described above.
  return timegm(&t2);
}


bool cpl::util::simple_scheduler::action( double const& t ) {

  if( t_last <= t && t < t_last + dt ) { return false ; }
  t_last = t ; return true ;

}


// The cast is necessary because std::isspace(int) is undefined for
// value other than EOF or unsigned char

void cpl::util::chop( std::string& s ) {

  std::size_t i ;
  for(
    i = s.size() ;
    i > 0 && std::isspace( static_cast< unsigned char >( s[ i - 1 ] ) ) ;
    --i
  )
  {}

  s.resize( i ) ;

}

void cpl::util::verify_alnum(std::string const& s, std::string const& extra) {
  for (char c : s) {
    if (!std::isalnum(c) && std::string::npos == extra.find(c)) {
      throw cpl::util::value_error(
            "invalid character in " + s 
          + ": must be alphanumeric or in " + extra);
    }
  }
}

////////////////////////////////////////////////////////////////////////
// UTF-8 stuff
////////////////////////////////////////////////////////////////////////

std::string cpl::util::utf8_tolower(std::string const& s) {
  auto ws = to_wstring(s);
  for (auto& c : ws) {
    c = std::tolower(c, utf8_locale);
  }
  return to_string(ws);
}

std::string cpl::util::utf8_toupper(std::string const& s) {
  auto ws = to_wstring(s);
  for (auto& c : ws) {
    c = std::toupper(c, utf8_locale);
  }
  return to_string(ws);
}

////////////////////////////////////////////////////////////////////////
// End UTF-8 stuff
////////////////////////////////////////////////////////////////////////


cpl::util::simple_scheduler::simple_scheduler( double const& dt_ )
: t_last( -std::numeric_limits< double >::max() )
{ reconfigure( dt_ ) ; }

void cpl::util::simple_scheduler::reconfigure( double const& dt_ )
{ always_assert( dt_ >= 0 ) ; dt = dt_ ; }


//
// File stuff.
//

using namespace cpl::util::file ;

// TODO: Better error reporting, factor out the common code
std::filebuf cpl::util::file::open_readbuf(
  std::string const& file                ,
  std::string      & which               ,
  std::vector< std::string > const& path
) {

  std::filebuf ret;

  std::string tried ;

  for( unsigned long i = 0 ; i < path.size() ; ++i ) {

    std::string const pathname = path[ i ] + "/" + file ;

    ret.open
      ( pathname.c_str() , std::ios_base::in | std::ios_base::binary ) ;

    if( !ret.is_open() )
    { tried += " " + pathname + ": " + std::strerror( errno ) ; }
    else { which = pathname ; return ret ; }

  }

  ret.open
    ( file.c_str() , std::ios_base::in | std::ios_base::binary ) ;

  if( !ret.is_open() )
  { tried += " " + file + ": " + std::strerror( errno ) ; }
  else { which = file ; return ret ; }

  assert( !ret.is_open() ) ;

  throw std::runtime_error
  ( "couldn't open " + file + " for reading:" + tried ) ;

}


std::filebuf
cpl::util::file::open_writebuf(
    std::string const& file ,
    std::ios_base::openmode const mode ) {

  std::filebuf ret;

  ret.open( file.c_str() , mode ) ;

  if( !ret.is_open() ) {

    throw std::runtime_error(
      "couldn't open " + file + " for writing: " + std::strerror( errno )
    ) ;

  }

  return ret ;

}


std::string cpl::util::file::basename(
  std::string const& name ,
  std::string const& suffix
) {

  if(
       name.size() >= suffix.size()
    && std::equal( name.end() - suffix.size() , name.end() , suffix.begin() )
  )
  { return std::string( name.begin() , name.end() - suffix.size() ) ; }
  else
  { return name ; }

}


cpl::util::file::logfile_manager::logfile_manager(
    long const n,
    std::string const& basename ,
    double const utc )
  : basename( basename ) ,
    current_day( cpl::util::day_number( utc ) ) ,
    q( n ) ,
    os( newfile( utc ) )
{ }

cpl::util::file::owning_ofstream 
cpl::util::file::logfile_manager::newfile( double const utc ) {
  std::string const name = filename( utc ) ;
  q.add( name ) ;
  // Append
  return cpl::util::file::open_write( name , 
      std::ios_base::binary | std::ios_base::out | std::ios_base::app ) ;
}

bool cpl::util::file::logfile_manager::update( double const utc ) {
  long const new_day = cpl::util::day_number( utc ) ;
  if( new_day > current_day ) {
    current_day = new_day ;
    os = newfile( utc ) ;
    return true ;
  } else {
    return false ;
  }
}

std::string cpl::util::file::logfile_manager::filename(
    double const utc ) const {
  return basename + "." + cpl::util::format_date( utc ) ;
}
