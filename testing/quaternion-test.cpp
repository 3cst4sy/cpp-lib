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
// Test harness for quaternions and geometry.
//

#include <iostream>
#include <limits>

#include "cpp-lib/matrix-wrapper.h"
#include "cpp-lib/quaternion.h"
#include "cpp-lib/geometry.h"
#include "cpp-lib/math-util.h"


using namespace cpl::math ;
using namespace cpl::matrix ;

vector_3_t const
random_vector() {

  vector_3_t ret ;
  ret( 0 ) = std::rand() ;
  ret( 1 ) = std::rand() ;
  ret( 2 ) = std::rand() ;

  return ret ;

}


vector_3_t const
random_unit_vector() {

  vector_3_t v = random_vector() ;
  return 1 / norm_2( v ) * v ;

}



quaternion<> const random_unit_quaternion() {

  quaternion<> ret( std::rand() , std::rand() , std::rand() , std::rand() ) ;
  normalize( ret ) ;
  return ret ;

}


// Interactively test rotation

void interactive() {

  vector_3_t a ;
  double theta ;
  long n ;

  while( 1 ) {
    std::cout << "enter theta (degrees), axis, n: " ;
    std::cin  >> theta >> a( 0 ) >> a( 1 ) >> a( 2 ) >> n ;

    theta *= pi / 180 ;

    if( !std::cin ) return ;

    quaternion<> q = rotation_quaternion( theta , a ) ;
    vector_3_t v = random_unit_vector() ;

    for( long i = 0 ; i < n ; ++i ) {

      std::cout << v ; 
      v = rotation( q , v ) ;

    }

  }

}





matrix_3_t const yaw_pitch_roll( euler_angles<> const& ea ) {

  double const cpsi   = std::cos( ea.psi   ) ;
  double const spsi   = std::sin( ea.psi   ) ;
  double const ctheta = std::cos( ea.theta ) ;
  double const stheta = std::sin( ea.theta ) ;
  double const cphi   = std::cos( ea.phi   ) ;
  double const sphi   = std::sin( ea.phi   ) ;

  matrix_3_t yaw   = zero<3>();
  matrix_3_t pitch = zero<3>();
  matrix_3_t roll  = zero<3>();

  yaw( 0 , 0 ) =  cpsi ; yaw( 0 , 1 ) = spsi ;
  yaw( 1 , 0 ) = -spsi ; yaw( 1 , 1 ) = cpsi ;
  yaw( 2 , 2 ) = 1 ;

  pitch( 0 , 0 ) = ctheta ; pitch( 0 , 2 ) = -stheta ;
  pitch( 2 , 0 ) = stheta ; pitch( 2 , 2 ) =  ctheta ;
  pitch( 1 , 1 ) = 1 ;

  roll( 1 , 1 ) =  cphi ; roll( 1 , 2 ) = sphi ;
  roll( 2 , 1 ) = -sphi ; roll( 2 , 2 ) = cphi ;
  roll( 0 , 0 ) = 1 ;

  return roll * pitch * yaw ;

}


//
// Return ``deviation'' of matrix from an orthogonal matrix.
//

double ortho_dev( matrix_3_t const& C ) {

  return (C * transpose(C) - identity<3>()).norm() ;

}



int main() {

  std::cout << "Test harness for quaternions and geometry.\n" ;

  // interactive() ;

  std::cout << "Testing sphere_surface_frame().\n" ;

  double v_res_max = 0 ;

  for( int i = 0 ; i < 10000 ; ++i ) {

    vector_3_t x = random_vector() ;
    matrix_3_t const C = sphere_surface_frame( x ) ;

    // Check deviation from orthogonal matrix.
    v_res_max = std::max( ortho_dev( C ) , v_res_max ) ;
    // Check deviation from det = 1.
    v_res_max = std::max( std::fabs( 1 - determinant<3>( C ) ) , v_res_max ) ;

  }

  std::cout << "max residual: "
            << v_res_max << '\n' ;


  std::cout << "Testing spherical <-> cartesian coordinate conversion.\n" ;

  v_res_max = 0 ;

  for( int i = 0 ; i < 10000 ; ++i ) {

    double r ;
    double theta ;
    double phi ;

    vector_3_t x = random_vector() ;

    cartesian_to_spherical( x , r , theta , phi ) ;

    vector_3_t y = r * spherical_to_cartesian( theta , phi ) ;

    double const v_res = norm_2( x - y ) ;

    // std::cout << "vector residual: " << v_res << '\n' ;

    v_res_max = std::max( v_res , v_res_max ) ;

  }

  std::cout << "max residual: "
            << v_res_max << '\n' ;


  std::cout << "Testing quaternion rotation wrt Euler angle rotation.\n" ;

  v_res_max = 0 ;

  for( int i = 0 ; i < 10000 ; ++i ) {

    vector_3_t r = random_unit_vector() ;

    euler_angles<> const ea( std::rand() , std::rand() , std::rand() ) ;

    vector_3_t v1 = yaw_pitch_roll( ea ) * r ;
    vector_3_t v2 = rotation( ea , r ) ;

    double const v_res = norm_2( v1 - v2 ) ;

    // std::cout << "vector residual: " << v_res << '\n' ;

    v_res_max = std::max( v_res , v_res_max ) ;

  }

  std::cout << "max value (should be around double epsilon = "
            << std::numeric_limits< double >::epsilon()
            << "): "
            << v_res_max << '\n' ;

  std::cout << "Testing quaternion -> dcm conversion.\n" ;

  v_res_max = 0 ;

  for( int i = 0 ; i < 10000 ; ++i ) {

    vector_3_t   const v = random_unit_vector()     ;
    quaternion<> const q = random_unit_quaternion() ;

    vector_3_t const v1 = rotation( q , v ) ;
    vector_3_t const v2 = make_dcm( q ) * v ;

    double const v_res = norm_2( v1 - v2 ) ;

    v_res_max = std::max( v_res , v_res_max ) ;

  }

  std::cout << "max value (should be around double epsilon = "
            << std::numeric_limits< double >::epsilon()
            << "): "
            << v_res_max << '\n' ;


  std::cout << "Testing DCM -> euler angles -> quaternion -> DCM.\n" ;

  v_res_max = 0 ;

  for( int i = 0 ; i < 10000 ; ++i ) {

    quaternion<> const q = random_unit_quaternion() ;
    matrix_3_t const C1 = make_dcm( q ) ;
    euler_angles<> const ea = make_euler_angles( C1 ) ;
    quaternion<> const q1 = make_quaternion( ea ) ;
    matrix_3_t const C2 = make_dcm( q1 ) ;

    v_res_max = std::max( (C2 - C1).lpNorm<Eigen::Infinity>() , v_res_max ) ;

  }

  std::cout << "max residual: "
            << v_res_max << '\n' ;


  std::cout << "Testing quaternion <-> Euler angles conversion.\n" ;

  double q_res_max          = 0 ;
  double change_psi_max     = 0 ;
  double change_psi_max_dcm = 0 ;

  for( int i = 0 ; i < 10000 ; ++i ) {

    quaternion  <> const q1 = random_unit_quaternion(    ) ;
    euler_angles<> const ea = make_euler_angles     ( q1 ) ;
    quaternion  <>       q2 = make_quaternion       ( ea ) ;

    {
      double const res = std::min( abs( q1 - q2 ) , abs( q1 + q2 ) ) ;
      q_res_max = std::max( res , q_res_max ) ;
    }

    {
      double const psi = ea.psi ;
      change_psi( q2 , psi + 1 ) ;
      change_psi( q2 , psi     ) ;

      {
        double const res = std::min( abs( q1 - q2 ) , abs( q1 + q2 ) ) ;
        change_psi_max = std::max( res , change_psi_max ) ;
      }
    }

    {
      matrix_3_t const C = make_dcm( q1 ) ;

      matrix_3_t C_mod = C ;
      euler_angles<> const ea = make_euler_angles( C ) ;
      change_psi( C_mod , ea.psi + 1 ) ;
      change_psi( C_mod , ea.psi     ) ;

      change_psi_max_dcm =
        std::max( ( C_mod - C ).lpNorm<Eigen::Infinity>() ,
                  change_psi_max_dcm ) ;
    }

  }

  std::cout << "max conversion residual:   "
            << q_res_max
            << '\n' ;
  std::cout << "max change_psi() residual: "
            << change_psi_max
            << '\n' ;
  std::cout << "max change_psi() residual (DCM): "
            << change_psi_max_dcm
            << '\n' ;

  std::cout
    << "Testing rotation_quaternion():\n"
    << "n times 2pi / n about random axis.\n" ;

  for( long n = 7 ; n <= 10000000 ; n *= 2 ) {

    std::cout << "n = " << n << " ... " ;
    double const theta = 2 * pi / n ;

    vector_3_t const a  = random_unit_vector() ;
    vector_3_t const v0 = random_unit_vector() ;
    vector_3_t       v  = v0                   ;

    quaternion<> q = rotation_quaternion( theta , a ) ;

    for( long i = 0 ; i < n ; ++i )
    { v = rotation( q , v ) ; }

    // Now v should again be v0.
    std::cout << "residual: " << norm_2( v - v0 ) << '\n' ;

  }

}
