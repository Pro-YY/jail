# how to build

prepare
```
apt install libseccomp-dev libcap-dev # for ubuntu/debian
```

host setting
```
sysctl -w net.ipv4.ip_forward=1
iptables -t nat -A POSTROUTING -s 172.17.0.0/16 -j MASQUERADE
```

build (release)
```
mkdir -p build/release
make release
```

# run

1. simple demo
```
./jail /bin/bash
```

2. with customized rootfs
```
./jail --root=my-rootfs --name=j1 --base=/data  -- /bin/bash
```


# parameters

customized rootfs

customized ip address

customized base dir

environment variables

timeout: --timeout 10

detached process: --detach

# features
- namespace-based process isolation
- capability dropping
- syscall filtering
- cgroup/rlimit resource limit

