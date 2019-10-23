# how to build

### Ubuntu 18.04
prepare
```
apt install libseccomp-dev libcap-dev

sysctl -w net.ipv4.ip_forward=1
iptables -t nat -A POSTROUTING -s 172.17.0.0/16 -j MASQUERADE
```

build
```
mkdir -p build/release
make release
```

# run

1.
```
./jail /bin/bash
```

2. with customized rootfs
```
./jail --root=my-rootfs --name=j1 --base=/data  -- /bin/bash
```


# params
customized rootfs
specify ip address
customized base dir
environment variables
timeout: --timeout 10
detached process: --detach

# features
namespace-based process isolation
capability dropping
syscall filtering
cgroup/rlimit resource limit

