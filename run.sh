#!/bin/bash

# set prefix for install directories
export LDASOFT_PREFIX='C:/GithubProjects/ldasoft_prefix'

# build codes
./install.sh ${LDASOFT_PREFIX}

# add location of binaries to PATH 
export PATH=${LDASOFT_PREFIX}/bin:${PATH}
