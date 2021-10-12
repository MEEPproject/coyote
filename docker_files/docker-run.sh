#!/bin/bash
docker run --name coyote-cont-ubuntu-20 -dit  -e HOST_USER_ID=$(id -u) -e HOST_USER_GID=$(id -g) -v ${HOME}/MEEP:/${HOME} --entrypoint=/bin/bash ubuntu-20-coyote:1.0
