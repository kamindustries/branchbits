git init
git submodule init
git submodule update

curl -O https://raw.githubusercontent.com/younkhg/alproj/master/CMakeLists.txt
curl -O https://raw.githubusercontent.com/younkhg/alproj/master/run.sh
curl -O https://raw.githubusercontent.com/younkhg/alproj/master/distclean

chmod 750 run.sh
chmod 750 distclean

mkdir -p cmake_modules
cd cmake_modules
curl -O https://raw.githubusercontent.com/younkhg/alproj/master/cmake_modules/make_dep.cmake
curl -O https://raw.githubusercontent.com/younkhg/alproj/master/cmake_modules/CMakeRunTargets.cmake
cd ..