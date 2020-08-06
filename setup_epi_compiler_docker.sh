#/bin/bash/!

 curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"

sudo apt-get update

sudo apt-get -y install docker docker-ce docker-ce-cli containerd.io
systemctl is-active --quiet docker || sudo service docker start #If docker inactive, activate
sudo docker build -t epi_compiler_docker docker_files
