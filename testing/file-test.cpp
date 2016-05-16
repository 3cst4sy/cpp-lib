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

#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>

#include "cpp-lib/assert.h"
#include "cpp-lib/command_line.h"
#include "cpp-lib/util.h"
#include "cpp-lib/sys/util.h"
#include "cpp-lib/sys/file.h"

#include "boost/lexical_cast.hpp"
#include "boost/filesystem.hpp"

using namespace cpl::util::file ;
using namespace cpl::util       ;

const cpl::util::opm_entry options[] = {
  cpl::util::opm_entry("cat"        , cpl::util::opp(false, 'c')),
  cpl::util::opm_entry("watch"      , cpl::util::opp(false, 'w')),
  cpl::util::opm_entry("logmanager" , cpl::util::opp(false, 'l')),
  cpl::util::opm_entry("tee"        , cpl::util::opp(false, 't')),
  cpl::util::opm_entry("ncalls"     , cpl::util::opp(true , 'n')),
  cpl::util::opm_entry("fileops"    , cpl::util::opp(false, 'f')),
};

void usage(std::string const& prog) {
  std::cerr << prog << " --cat | --watch | --tee | --fileops | --logmanager [ --ncalls <repetitions> ] <filename...>\n";
}

// Watch the current directory from another terminal while this is running.
// There should be a maximum of 5 files at any given time.
void test_logfile_manager() {
  const double now = cpl::util::utc() ;
  cpl::util::file::logfile_manager lm(5, "test-logfile", now);

  for (int i = 0; i < 20; ++i) {
    // Fast-forward
    lm.update(now + i * cpl::units::day());
    lm << "Hi there!\nThis is file #" << i << ".\n";
    cpl::util::sleep(1);
  }
}


// Multiply input to n files given on command line.
void test_tee(std::istream& is, cpl::util::command_line& cl) {

  std::vector<cpl::util::file::owning_ofstream> files;

  std::string filename;
  std::vector<std::string> filenames;
  while (cl >> filename) {
    filenames.push_back(filename);
    files.push_back(cpl::util::file::open_write(filename));
  }

  std::string line;
  while (std::getline(is, line)) {
    for (unsigned i = 0; i < files.size(); ++i) {
      files[i] << line << std::endl;
      if (!files[i]) {
        throw std::runtime_error("Write failed to " + filenames[i]);
      }
    }
  }
}

void cat(std::string const& name, std::ostream& os) {
  auto is = open_read(name);
  std::string line;
  os << "Contents of " << name << ":" << std::endl;
  while (std::getline(is, line)) {
    os << line << '\n';
  }
}

void test_fileops(std::ostream& os) {
  std::string const name = "file-test-473856y71234";
  std::string const name2 = "file-test-473856y71234-2";
  std::string const name3 = "file-test-473856y71234-3";
  boost::filesystem::path p = boost::filesystem::temp_directory_path();
  cpl::util::file::chdir(p.string());

  verify_throws("unlink", cpl::util::file::unlink, name, false);

  // Can't cd to nonexisting
  verify_throws("chdir", cpl::util::file::chdir, name);

  // Create test file
  {
    auto os = open_write(name);
    os << "Hi there" << std::endl;
  }
  // Can't cd to file
  verify_throws("chdir", cpl::util::file::chdir, name);

  cpl::util::file::link(name, name2);
  cat(name2, os);
  cpl::util::file::unlink(name);
  verify_throws("unlink", cpl::util::file::unlink, name, false);

  cat(name2, os);

  cpl::util::file::rename(name2, name3);
  verify_throws("unlink", cpl::util::file::unlink, name2, false);
  cat(name3, os);

  cpl::util::file::unlink(name3);
  // Ignore missing file
  cpl::util::file::unlink(name3, true);

  verify_throws("unlink", cpl::util::file::unlink, name3, false);
}

int main( int , char const* const* const argv ) {

  try {

  cpl::util::command_line cl( options , options + size( options ) , argv ) ;

  if (cl.is_set("logmanager")) {
    test_logfile_manager();
    return 0;
  }

  if (cl.is_set("tee")) {
    test_tee(std::cin, cl);
    return 0;
  }

  if (cl.is_set("fileops")) {
    test_fileops(std::cout);
    return 0;
  }

  std::string name ;

  if (!(cl >> name)) {
    usage(argv[0]);
    return 1;
  }

  if (cl.is_set("watch")) {
  
  file_name_watcher fw( name ) ;

  const long n = cl.is_set("ncalls") ? 
      boost::lexical_cast<long>(cl.get_arg("ncalls")) : 1000;

  double const t = utc() ;
  for( long i = 0 ; i < n ; ++i ) 
  { fw.modified() ; }

  double const tt = utc() ;

  std::cout << "Elapsed time: " << tt - t << " seconds." << std::endl ;
  std::cout << "Calls per second: " << n / ( tt - t ) << std::endl ;

  while( 1 ) {

	cpl::util::sleep( 1 ) ;

	std::cout << "File modified: " << fw.modified() << std::endl ;

  }

  } else if (cl.is_set("cat")) {
    cat(name, std::cout);
  } else {
    usage(argv[0]);
    return 1;
  }


  } catch( std::exception const& e )
  { die( std::string("Error: ") + e.what() ) ; }

}
