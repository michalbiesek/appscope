FROM nginx:latest

RUN apt update && apt install -y \
  curl \
  wget \
  emacs \
  gdb \
  net-tools \
  vim \
  apache2-utils \
  gnupg \
  libpq5 \
&& rm -rf /var/lib/apt/lists/*

COPY scope /usr/bin/scope
COPY config/conf_tcp.yml /test/conf_tcp.yml
COPY config/conf_unix.yml /test/conf_unix.yml
COPY config/nginx.conf /etc/nginx/nginx.conf
COPY gdbinit /root/.gdbinit



