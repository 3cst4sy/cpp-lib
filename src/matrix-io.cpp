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


#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include "cpp-lib/util.h"

using namespace cpl::math ;

/*void cpl::math::to_file(
  std::vector< std::vector< double > > const& v ,
  std::string const& filename ,
  bool do_Transpose
) {

  auto os = cpl::util::file::open_write( filename ) ;

  os.precision( MATRIX_PRECISION ) ;

  matrix< double , vector_vector > m( v ) ;

  if( do_Transpose ) { os << transpose( m ) ; }
  else               { os <<            m   ; }

}*/
