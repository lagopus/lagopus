%define kmod_name ptree

Name:           %{kmod_name}-devel
Version:        1.0.1
Release:        2%{?dist}
Summary:        %{kmod_name} kernel module(s)

Group:          System/Kernel
License:        GPLv2
Source0:        %{kmod_name}-%{version}.tar.bz2
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildRequires:  %kernel_module_package_buildreqs
ExclusiveArch:  x86_64

%undefine _enable_debug_packages

%{?kversion:%define kernel_version %kversion}
%{!?kflavors:%define kflavors default}

%kernel_module_package -n %{kmod_name} %kflavors

%description
An implementation of generic Patricia Tree structure.

%prep
%setup -n %{kmod_name}-%{version}

%build
%{__make} modules %{?_smp_mflags}

%install
export INSTALL_MOD_PATH=$RPM_BUILD_ROOT
export INSTALL_MOD_DIR=extra/%{kmod_name}
%{__make} modules_install
%{__install} -d %{buildroot}%{_includedir}/linux
%{__install} -m 0644 include/linux/ptree.h %{buildroot}%{_includedir}/linux/.
%{__install} -d %{buildroot}%{_sharedstatedir}/%{kmod_name}/%{version}/%{kernel_version}/
%{__install} -m 0644 Module.symvers %{buildroot}%{_sharedstatedir}/%{kmod_name}/%{version}/%{kernel_version}/.

%clean
rm -rf $RPM_BUILD_ROOT

%files
%{_includedir}/linux/ptree.h
%{_sharedstatedir}/%{kmod_name}/%{version}/%{kernel_version}/Module.symvers
