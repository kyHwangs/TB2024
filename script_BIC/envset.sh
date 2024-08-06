#! /bin/bash

# When using CVMFS (no X11)
# source /cvmfs/sft.cern.ch/lcg/views/LCG_105/arm64-mac13-clang150-opt/setup.sh

# When using local ROOT (X11 activated)
source /Users/skku_server/root-6.32.02/install_dir/bin/thisroot.sh

export INSTALL_DIR_PATH=$PWD/install

export PATH=$PATH:$INSTALL_DIR_PATH/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSTALL_DIR_PATH/lib
export DYLD_LIBRARY_PATH=$INSTALL_DIR_PATH/lib
export PYTHONPATH=$PYTHONPATH:$INSTALL_DIR_PATH/lib
export YAMLPATH=/cvmfs/sft.cern.ch/lcg/releases/yamlcpp/0.6.3-d05b2/arm64-mac13-clang150-opt/lib
