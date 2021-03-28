FROM ubuntu:20.04

ENV USER=bscuser USER_ID=1000 USER_GID=1000

RUN apt-get update && \
    apt-get upgrade --yes && \
    apt-get install --yes wget &&\
    wget https://admin.hca.bsc.es/epi/ftp/llvm-EPI-0.7-release-toolchain-cross-2020-03-09-1237.tar.bz2 && \
    tar -xjf llvm-EPI-0.7-release-toolchain-cross-2020-03-09-1237.tar.bz2 && \
    rm llvm-EPI-0.7-release-toolchain-cross-2020-03-09-1237.tar.bz2 && \
    wget https://admin.hca.bsc.es/epi/ftp/llvm-EPI-release-toolchain-cross-2020-05-12-1155.tar.bz2 && \
    tar -xjf llvm-EPI-release-toolchain-cross-2020-05-12-1155.tar.bz2 && \
    rm llvm-EPI-release-toolchain-cross-2020-05-12-1155.tar.bz2

RUN apt-get -y install sudo
RUN apt-get -y install software-properties-common vim byobu

# now creating user
RUN groupadd --gid "${USER_GID}" "${USER}" && \
    useradd \
      --uid ${USER_ID} \
      --gid ${USER_GID} \
      --create-home \
      --shell /bin/bash \
      ${USER}

RUN echo "bscuser:bscuser" | chpasswd
RUN adduser ${USER} sudo

COPY user-mapping.sh /
RUN  chmod u+x user-mapping.sh

ENTRYPOINT ["/user-mapping.sh"]
