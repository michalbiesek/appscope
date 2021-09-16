FROM ubuntu:21.04

RUN apt update && apt install -y \
  curl \
  wget \
  emacs \
  gdb \
  net-tools \
  vim \
  apache2-utils \
  nginx \
  gnupg \
  libpq5 \
  gcc \
&& rm -rf /var/lib/apt/lists/*

COPY fluent-bit /usr/bin/fluent-bit
COPY parsers.conf /test/parsers.conf
COPY gdbinit /root/.gdbinit


#RUN wget -qO - https://packages.fluentbit.io/fluentbit.key | apt-key add -
#RUN echo "deb https://packages.fluentbit.io/ubuntu/bionic bionic main" > /etc/apt/sources.list 
#RUN apt update && apt install -y td-agent-bit
