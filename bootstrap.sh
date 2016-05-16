#!/bin/sh

mkdir download
mkdir lib

wget http://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.bz2 -O download/boost_1_60_0.tar.bz2
wget http://bitbucket.org/eigen/eigen/get/3.2.8.tar.bz2 -O download/eigen_3.2.8.tar.bz2
wget http://download.savannah.gnu.org/releases/pngpp/png++-0.2.9.tar.gz  -O download/png++-0.2.9.tar.gz
wget https://sourceforge.net/projects/libpng/files/libpng16/1.6.21/libpng-1.6.21.tar.gz  -O download/libpng-1.6.21.tar.gz

tar xvf download/boost_1_60_0.tar.bz2 -C lib
tar xvf download/eigen_3.2.8.tar.bz2 -C lib
tar xvf download/png++-0.2.9.tar.gz -C lib
tar xvf download/libpng-1.6.21.tar.gz -C lib

cd lib/boost_1_60_0/
./bootstrap.bat
./b2 --toolset=msvc variant=release link=static threading=multi runtime-link=static stage

cd lib/libpng-1.6.21/

