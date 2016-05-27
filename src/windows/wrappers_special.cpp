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
// Special Windoze stuff requiring linking with advapi32.dll, user32.dll
// and stuff like that.
//

#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>

#include <cmath>

#include "cpp-lib/util.h"
#include "cpp-lib/sys/util.h"
#include "cpp-lib/windows/wrappers.h"
  

using namespace cpl::detail_ ;


namespace {


// Don't ask why it works, it just does. 

void exit_windows( bool reboot ) {

  HANDLE h ; 
  TOKEN_PRIVILEGES tkp ; 
   
  if( 
    !OpenProcessToken( 
      GetCurrentProcess() , 
      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY , 
      &h
     )
  ) { last_error_exception( "OpenProcessToken" ) ; }
   
  LookupPrivilegeValue( 0 , SE_SHUTDOWN_NAME , &tkp.Privileges[0].Luid ) ; 
   
  tkp.PrivilegeCount = 1 ;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED ; 
   
  AdjustTokenPrivileges( h , FALSE , &tkp , 0 , 0 , 0 ) ; 
   
  if( ERROR_SUCCESS != GetLastError() )
  { last_error_exception( "AdjustTokenPrivileges" ) ; }

  UINT const f = ( reboot ? EWX_REBOOT : EWX_POWEROFF ) | EWX_FORCE ;
   
  if( !ExitWindowsEx( f , 0 ) ) 
  { last_error_exception( "ExitWindowsEx" ) ; }

}

} // anonymous namespace


void cpl::util::reboot  () { exit_windows( true  ) ; }
void cpl::util::poweroff() { exit_windows( false ) ; }
