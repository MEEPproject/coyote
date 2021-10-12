#!/bin/bash
docker run --name epi_compiler -dit -e HOST_USER_ID=$(id -u) -e HOST_USER_GID=$(id -g) -v ${HOME}:/${HOME} --entrypoint=/user_mapping.sh epi_compiler_docker
