Summary:        Library for generating random numbers using the RdRand instruction on Intel CPUs
Name:           RdRand
Version:        1.0.0rc1
Release:        1%{?dist}
License:        LGPLv2+
Group:          Applications/System
URL:            http://github.com/BroukPytlik/%{name}
Source0:        https://github.com/BroukPytlik/%{name}/archive/%{version}.tar.gz
BuildRequires: autoconf libtool

%description
RdRand is an instruction for returning random numbers from an on-chip hardware random number generator. It's available on Ivy Bridge and Haswell processors and is part of the Intel 64 and IA-32 instruction set architectures. The hardware random number generator is compliant with security and cryptography standards such as NIST SP 800-90A, FIPS 140-2 and ANSI X9.82. 
This library offers high level functions to fill the buffer with random numbers using the various methods and offers the procedures focused on speed and security. There is also a user space utility to produce the stream of random numbers.


%package devel
Summary:        Development files for the RdRand
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description devel
Headers and shared object symbolic links for the RdRand


%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT INSTALL="%{__install} -p"
rm -f $RPM_BUILD_ROOT{%{_libdir}/librdrand.la,%{_libdir}/librdrand/include/rdrandconfig.h,%{_libdir}/pkgconfig/librdrand.pc}
#rm -f $RPM_BUILD_ROOT{% {_libdir}/libaa.la,% {_infodir}/dir}

# clean up multilib conflicts
#touch -r NEWS $RPM_BUILD_ROOT% {_datadir}/rdrand-gen # $RPM_BUILD_ROOT% {_datadir}/librdrand.m4

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc README COPYING ChangeLog NEWS
%{_bindir}/rdrand-gen
%{_mandir}/man7/rdrand-gen.7.gz
%{_libdir}/librdrand.so.*

%files devel
%defattr(-,root,root,-)
%{_mandir}/man3/librdrand.3.gz
%{_includedir}/librdrand.h
%{_libdir}/librdrand.so
#% {_infodir}/librdrand.info*
#% {_datadir}/aclocal.m4

%changelog
* Wed Dec 04 2013 Jan Tulak <jan@tulak.me> - 1.0.0-1
- Created.
