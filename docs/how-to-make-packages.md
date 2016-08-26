<!-- -*- mode: markdown ; coding: us-ascii-dos -*- -->

How To Make The Binary Packages
======================================

Introduction
------------
This is the file that you describe about creating the packages.

Make Debian/Ubuntu packages (.deb)
---------------------------
## Recommended
* Ubuntu 12.04
* Ubuntu 13.04
* Ubuntu 13.10

## Related files
### Common
 * mk/pkg_param.conf

### For .deb
 * debian/Makefile.in : A make file.
 * debian/changelog : A changelog file. You want to edit using `dch` command.
 * debian/compat : The debhelper compatibility level.
 * debian/control_common.inc : The control file for common.
 * debian/control_dpdk.inc : The control file for dpdk version.
 * debian/control_raw_socket.inc : The control file for raw socket version.
 * debian/copyright.in : The copyright file.
 * debian/lagopus-DATAPLANE.dirs : List of install directories.
 * debian/lagopus-DATAPLANE.install : List of install directories.
 * debian/lagopus-DATAPLANE.postrm.in : The hook script for postrm. It is executed before the removal.
 * debian/lagopus-DATAPLANE.prerm: The hook script for prerm. It is executed after the removal.
 * debian/rules.in : The rules file.

## Dependency
* Package manager for debian
* The debhelper tool suite
* Scripts to ease the lives of Debian developers
* Simple tool to install deb files
* SNMP (Simple Network Management Protocol)
* (OPTIONAL) Intel DPDK (Intel Data Plane Development Kit)  
  cf.) http://www.intel.com/content/www/us/en/intelligent-systems/intel-technology/intel-dpdk-getting-started-guide.html

You execute the following command:  
e.g.)

```
 % sudo apt-get install dpkg-dev debhelper devscripts gdebi libsnmp-dev

 # Ubuntu 12.*
 % sudo apt-get install python2.7-dev

 # Ubuntu 13.*
 % sudo apt-get install libpython2.7-dev
```

## Operation to create the lagopus packages
### 1. Edit `mk/pkg_param.conf` file.
You write the following fields in `mk/pkg_param.conf` file.
More information about the fields, see the following URL:  
cf.) https://www.debian.org/doc/debian-policy/ch-controlfields.html

```
 % cd </PATH/TO/LAGOPUS_TOP_DIR>
 % vi mk/pkg_param.conf
```

#### MUST
  * PKG_NAME : The package name (Same as `Source` in debian/control file.)
  * PKG_VERSION(auto generate) : The package version (Same as `Package` in debian/control file.).
  * PKG_REVISION : The package revision number (Same as `Package` in debian/control file.)
  * PKG_DATE(auto generate) : Creation date of the packages
  * PKG_MAINTAINER : The package maintainer's name and email address (Same as `Maintainer` in debian/control file.)
  * PKG_SECTION : Classification of the packages (Same as `Section` in debian/control file.)
  * PKG_PRIORITY : Priority of the packages (Same as `Priority` in debian/control file.)

#### OPTIONAL
  * PKG_DESCRIPTION_RAW_SOCKET : The package description for raw socket version (Same as `Description` in debian/control file.)
  * PKG_DESCRIPTION_DPDK : The package description for DPDK version (Same as `Description` in debian/control file.)
  * PKG_HP_URL : The URL of the web site (Same as `Homepage` in debian/control file.)
  * PKG_VCS : The VCS repository (Same as `Vcs-Git` in debian/control file.)
  * PKG_VCS_URL : The URL of a web interface for browsing the VCS repository (Same as `Vcs-Browser` in debian/control file.)
  * PKG_REMOVE_DIRS : This is the files to be deleted if it is with the `--remove` option
  * PKG_REMOVE_FILES : This is the directories to be deleted if it is with the `--remove` option
  * PKG_PURGE_DIRS : This is the files to be deleted if it is with the `--purge` option
  * PKG_PURGE_FILES : This is the directories to be deleted if it is with the `--purge` option

e.g.)

```
 PKG_NAME="lagopus"
 PKG_VERSION="1.0.0" # auto generate.
 PKG_REVISION="1"
 PKG_DATE="Fri, 6 Nov 2015 00:00:00 +0000" # auto generate.
 PKG_MAINTAINER="lagopus <lagopus@lagopus.org>"
 PKG_SECTION="unknown"
 PKG_PRIORITY="extra"
 PKG_DESCRIPTION_RAW_SOCKET="raw socket version."
 PKG_DESCRIPTION_DPDK="DPDK version."
 PKG_HP_URL="http://www.lagopus.org"
 PKG_VCS="git://lagopus.org"
 PKG_VCS_URL=
 PKG_REMOVE_DIRS='/usr/sbin/of_config /usr/sbin/ovsdb'
 PKG_REMOVE_FILES=
 PKG_PURGE_DIRS=${PKG_REMOVE_DIRS}
 PKG_PURGE_FILES='/etc/lagopus/lagopus.dsl
                  /etc/lagopus/configuration.xml
                  /etc/lagopus/operational.xml
                  /etc/lagopus/ofconf_rsa
                  /etc/lagopus/ofconf_rsa.pub
                  /etc/lagopus/ofconf-passwords'
```

### 2. Execute configure.
* In the case of make the package for raw socket version:  
  You execute `configure` script with no options.

    ```
     % ./configure
    ```

* In the case of make the packages raw socket and DPDK version:  
  You execute `configure` command with `--with-dpdk-dir` option.  
  `RTE_SDK` is path to DPDK Directory.

    ```
     % ./configure --with-dpdk-dir=${RTE_SDK}
    ```

### 3. Execute make.
You execute the following commands.

```
 % make pkg-deb
 % ls pkg/*.deb
 pkg/lagopus-raw-socket_1.0.0-1_amd64.deb [pkg/lagopus-dpdk_1.0.0-1_amd64.deb]
```

## Install and Uninstall the lagopus package
### Install

e.g.)

* Auto resolve dependencies.

    ```
     % sudo apt-get install gdebi
     % sudo gdebi pkg/lagopus-{raw-socket,dpdk}_1.0.0-1_amd64.deb
    ```

* Manual resolve dependencies.

    ```
     % sudo dpkg -i pkg/lagopus-{raw-socket,dpdk}_1.0.0-1_amd64.deb
    ```

### Uninstall
* In the case of `remove`:  
This means that the configuration file(/etc/...) is not deleted.  
e.g.)

    ```
     % sudo dpkg --remove lagopus-{raw-socket,dpdk}
    ```

* In the case of `purge`:  
e.g.)

    ```
     % sudo dpkg --purge lagopus-{raw-socket,dpdk}
    ```
