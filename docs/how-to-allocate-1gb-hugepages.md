# How to allocate 1GB hugepages on Ubnutn 16.04

## 1. Edit `/etc/default/grub`

This setting allocates 1GB * 16pages = 16GB hugepages on boot time.

- `/etc/default/grub`

```
GRUB_CMDLINE_LINUX_DEFAULT="default_hugepagesz=1G hugepagesz=1G hugepages=16"
```


## 2. Update GRUB


Run `update-grub` to apply the config to grub.

```console
$ sudo update-grub
```


## 3. Mount hugetlbfs

Prepare hugetlbfs individually for the host and each container.
In order to mount hugetlbfs, edit `/etc/fstab` like below.

- `/etc/fstab`

```
# for host
none /mnt/huge hugetlbfs pagesize=1G,size=4G 0 0
# for container
none /mnt/huge_c0 hugetlbfs pagesize=1G,size=1G 0 0
# none /mnt/huge_c1 hugetlbfs pagesize=1G,size=1G 0 0 # For another container, add the entry like this
```


## 4. Reboot

Reboot after the above setup.
Hugepages will be allocated on boot time.

After the reboot, check allocated hugepages collectly.

```console
$ grep Huge /proc/meminfo
AnonHugePages:      2048 kB
HugePages_Total:      16
HugePages_Free:       16
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:    1048576 kB
$ df
Filesystem     1K-blocks    Used Available Use% Mounted on
...
none             1048576       0   1048576   0% /mnt/huge_c0
none             4194304       0   4194304   0% /mnt/huge
```
