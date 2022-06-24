FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -y \
      curl \
      dnsutils \
      gdb \
      openssh-client \
      openssh-server \
      sshpass \
    && rm -rf /var/lib/apt/lists/*

# Create a user “sshuser” and group “sshgroup”
RUN groupadd sshgroup && useradd -ms /bin/bash -g sshgroup sshuser
RUN echo sshuser:sshuser | chpasswd 

RUN mkdir -p /opt/test-runner
COPY sshd/ssh_test.sh /opt/test-runner

RUN mkdir -p /etc/scope
COPY sshd/scope.yml /etc/scope

RUN service ssh start

COPY scope-profile.sh /etc/profile.d/scope.sh
COPY gdbinit /root/.gdbinit

RUN ln -s /opt/appscope/bin/linux/$(uname -m)/scope /usr/local/bin/scope && \
    ln -s /opt/appscope/bin/linux/$(uname -m)/ldscope /usr/local/bin/ldscope && \
    ln -s /opt/appscope/lib/linux/$(uname -m)/libscope.so /usr/local/lib/libscope.so

COPY sshd/scope-test /usr/local/scope/scope-test

COPY docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["test"]
