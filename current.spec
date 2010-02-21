Summary: An fdisk-like partitioning tool for GPT disks
Name: gdisk
Version: 0.6.4
Release: 1%{?dist}
License: GPLv2
URL: http://www.rodsbooks.com/gdisk
Group: Applications/System
Source: http://www.rodsbooks.com/gdisk/gdisk-0.6.4.tgz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
An fdisk-like partitioning tool for GPT disks. GPT
fdisk features a command-line interface, fairly direct
manipulation of partition table structures, recovery
tools to help you deal with corrupt partition tables,
and the ability to convert MBR disks to GPT format.

%prep
%setup -q

%build
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/sbin
install -Dp -m0755 gdisk $RPM_BUILD_ROOT/sbin
install -Dp -m0755 sgdisk $RPM_BUILD_ROOT/sbin
install -Dp -m0644 gdisk.8 $RPM_BUILD_ROOT/%{_mandir}/man8/gdisk.8
install -Dp -m0644 sgdisk.8 $RPM_BUILD_ROOT/%{_mandir}/man8/sgdisk.8

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root -)
%doc CHANGELOG COPYING README
/sbin/gdisk
/sbin/sgdisk
%doc %{_mandir}/man8*

%changelog
* Fri Feb 19 2010 R Smith <rodsmith@rodsbooks.com> - 0.6.4
- Created spec file for 0.6.4 release
