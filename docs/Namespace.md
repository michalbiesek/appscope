# Switch namespaces

TODO

## Design

TODO

## Start Edge container

```console
docker run --privileged -d -e CRIBL_EDGE=1 -p 9420:9420 -v /var/run/appscope:/var/run/appscope -v /var/run/docker.sock:/var/run/docker.sock  -v /:/hostfs:ro --restart unless-stopped --name cribl-edge cribl/cribl:next

# Optionally replace the AppScope cli to the Edge container - usefull for local development
docker cp ./bin/linux/x86_64/scope cribl-edge:/opt/cribl/bin
docker exec --user root cribl-edge chown root:root /opt/cribl/bin/scope
```

The command above allows to:

- [access Edge UI](http://localhost:9420/)
- Share the AppScope source listening socket: `/var/run/appscope/appscope.sock` between host and Edge container

## Docker container

```console
docker run --privileged --rm -p 6379:6379 --name redisAlpine -v /var/run/appscope:/var/run/appscope redis:alpine
```

## Podman container

```console
podman run -d --name redisPodman -p 6379:6379 -v /var/run/appscope:/var/run/appscope redis:latest
```

## [LXC](https://github.com/lxc/lxc)

```console
lxc launch ubuntu:20.04 lxc-example
<!-- Share the AppScope Source listening socket-->
lxc config device add lxc-example appscopeSocket disk source=/run/appscope path=/var/run/appscope/
<!-- Login to lxc container-->
lxc exec lxc-example bash
```

### Containerd - [ctr](https://github.com/containerd/containerd)

```console
sudo ctr run -d --mount type=bind,src=/var/run/appscope/,dst=/var/run/appscope/,options=rbind:rw docker.io/library/redis:alpine ctr_redis
```

### Containerd - [nerdctl](https://github.com/containerd/nerdctl.git) as root

```console
sudo nerdctl run -d -v /var/run/appscope/:/var/run/appscope/ --name nerdctl_root_redis redis:7.0.5
```
