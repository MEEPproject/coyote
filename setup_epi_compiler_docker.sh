#/bin/bash/!
sudo apt-get install docker
systemctl is-active --quiet docker || sudo service docker start #If docker inactive, activate
sudo docker build -t docker_epi_compiler docker_files
