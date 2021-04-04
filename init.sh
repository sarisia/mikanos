#!/bin/bash

workspace=/workspaces/mikanos
edk2=/home/vscode/edk2
current=$(pwd)

export CONF_PATH=${workspace}/edk2/Conf

ln -sf ${workspace}/MikanLoaderPkg ${edk2}

cd ${edk2}
source edksetup.sh
cd ${current}

unset -v workspace edk2 current
