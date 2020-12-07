#!/bin/bash
# cleanup cached Homebrew's subdirectories 
curdir=`pwd`
cd /usr/local/Homebrew/
if [[ -d Library/Taps/homebrew/homebrew-cask ]]; then
   rm -rf Library/Taps/caskroom/homebrew-cask
fi
for d in `find $(pwd) -type d -name .git`; do
   cd `dirname $d`
   git clean -fxd
done
brew cleanup
cd $curdir

brew update
brew install glew lz4 lzo libusb libmypaint
brew tap tcr/tcr
brew install clang-format
# from Homebrew 1.6.0 the old formula for obtaining Qt5.9.2 becomes invalid.
# so we start to use the latest version of Qt. (#1910)
brew install qt
# delete older qt versions and make sure to have only the latest
brew cleanup qt
# temp workaround to brew installed glew cmake info overriding glew lib detection
# which causes compiling issues later
if [ -L /usr/local/lib/cmake/glew ]
then
   echo "Symbolic link '/usr/local/lib/cmake/glew' detected. Removing to avoid glew lib detection issue!"
   ls -l /usr/local/lib/cmake/glew
   rm /usr/local/lib/cmake/glew
fi
