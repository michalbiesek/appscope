FROM cribl/cribl:3.1.0-RC4

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

COPY config/ /opt/cribl/local/cribl/
COPY gdbinit /root/.gdbinit

ENV CRIBL_NOAUTH=1

