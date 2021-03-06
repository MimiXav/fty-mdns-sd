#
#    fty-mdns-sd - This service manages network anouncement (mDNS) and discovery (DNS-SD)
#
#    Copyright (C) 2014 - 2018 Eaton
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif
%define SYSTEMD_UNIT_DIR %(pkg-config --variable=systemdsystemunitdir systemd)
Name:           fty-mdns-sd
Version:        1.0.0
Release:        1
Summary:        this service manages network anouncement (mdns) and discovery (dns-sd)
License:        GPL-2.0+
URL:            https://42ity.org
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  systemd-devel
BuildRequires:  systemd
%{?systemd_requires}
BuildRequires:  xmlto
BuildRequires:  gcc-c++
BuildRequires:  libsodium-devel
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel >= 3.0.2
BuildRequires:  malamute-devel >= 1.0.0
BuildRequires:  avahi-devel >= 0.6.31
BuildRequires:  fty-common-logging-devel
BuildRequires:  fty-proto-devel >= 1.0.0
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
fty-mdns-sd this service manages network anouncement (mdns) and discovery (dns-sd).

%package -n libfty_mdns_sd0
Group:          System/Libraries
Summary:        this service manages network anouncement (mdns) and discovery (dns-sd) shared library

%description -n libfty_mdns_sd0
This package contains shared library for fty-mdns-sd: this service manages network anouncement (mdns) and discovery (dns-sd)

%post -n libfty_mdns_sd0 -p /sbin/ldconfig
%postun -n libfty_mdns_sd0 -p /sbin/ldconfig

%files -n libfty_mdns_sd0
%defattr(-,root,root)
%{_libdir}/libfty_mdns_sd.so.*

%package devel
Summary:        this service manages network anouncement (mdns) and discovery (dns-sd)
Group:          System/Libraries
Requires:       libfty_mdns_sd0 = %{version}
Requires:       libsodium-devel
Requires:       zeromq-devel
Requires:       czmq-devel >= 3.0.2
Requires:       malamute-devel >= 1.0.0
Requires:       avahi-devel >= 0.6.31
Requires:       fty-common-logging-devel
Requires:       fty-proto-devel >= 1.0.0

%description devel
this service manages network anouncement (mdns) and discovery (dns-sd) development tools
This package contains development files for fty-mdns-sd: this service manages network anouncement (mdns) and discovery (dns-sd)

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libfty_mdns_sd.so
%{_libdir}/pkgconfig/libfty_mdns_sd.pc
%{_mandir}/man3/*
%{_mandir}/man7/*

%prep

%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-systemd-units
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%doc README.md
%{_bindir}/fty-mdns-sd
%{_mandir}/man1/fty-mdns-sd*
%config(noreplace) %{_sysconfdir}/fty-mdns-sd/fty-mdns-sd.cfg
%{SYSTEMD_UNIT_DIR}/fty-mdns-sd.service
%dir %{_sysconfdir}/fty-mdns-sd
%if 0%{?suse_version} > 1315
%post
%systemd_post fty-mdns-sd.service
%preun
%systemd_preun fty-mdns-sd.service
%postun
%systemd_postun_with_restart fty-mdns-sd.service
%endif

%changelog
