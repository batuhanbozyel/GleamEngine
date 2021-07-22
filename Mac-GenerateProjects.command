#!/bin/sh
chmod u+x $HOME/VulkanSDK/1.2.182.0/setup-env.sh
source $HOME/VulkanSDK/1.2.182.0/setup-env.sh

cd "`dirname "$0"`"
chmod u+x ./Mac-GenerateProjects.sh
./Mac-GenerateProjects.sh