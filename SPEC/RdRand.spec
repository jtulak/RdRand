Summary:        Library for generating random numbers using the RdRand instruction on Intel CPUs
Name:           RdRand
Version:        1.0.4
Release:        2%{?dist}
License:        LGPLv2+
Group:          Applications/System
URL:            http://github.com/BroukPytlik/%{name}
Source0:        https://github.com/BroukPytlik/%{name}/archive/%{version}.tar.gz
ExclusiveArch: %{ix86} x86_64 

%description
RdRand is an instruction for returning random numbers from an Intel on-chip 
hardware random number generator.RdRand is available in Ivy Bridge and later 
processors.

It uses cascade construction, combining a HW RNG operating at 3Gbps with CSPRNG
with all components sealed on CPU. The entropy source is a meta-stable circuit,
with unpredictable behavior based on thermal noise. The entropy is fed into 
a 3:1 compression ratio entropy extractor (whitener) based on AES-CBC-MAC. 
Online statistical tests are performed at this stage and only high quality 
random data are used as the seed for cryptograhically secure SP800-90 AES-CTR 
DRBG compliant PRNG. 
This generator is producing maximum of 512 128-bit AES blocks before it's 
reseeded. According to documentation the 512 blocks is a upper limit for 
reseed, in practice it reseeds much more frequently.

%package devel
Summary:        Development files for the RdRand
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
Headers and shared object symbolic links for the RdRand.


%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
make install DESTDIR=$RPM_BUILD_ROOT INSTALL="%{__install} -p"
rm -f $RPM_BUILD_ROOT{%{_libdir}/librdrand.la,%{_libdir}/librdrand/include/rdrandconfig.h}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%doc README COPYING ChangeLog NEWS
%{_bindir}/rdrand-gen
%{_mandir}/man7/rdrand-gen.7.*
%{_libdir}/librdrand.so.*

%files devel
%{_mandir}/man3/librdrand.3.*
%{_includedir}/librdrand.h
%{_libdir}/librdrand.so
%{_libdir}/pkgconfig/*

%changelog
* Thu Feb 6 2014 Jan Tulak <jan@tulak.me> - 1.0.4-2
- Removed %defattr from devel subpackage, removed commented out lines,
- man page suffix changed from .gz to .*

* Wed Feb 5 2014 Jan Tulak <jan@tulak.me> - 1.0.4-1
- ExclusiveArch, removed %clean, %defattr, added blank lines between changelogs.
- Removed temp files from package.

* Fri Jan 31 2014 Jirka Hladky <jhladky@redhat.com> - 1.0.2-2
- Fixed License, ExcludeArch and Requires for the devel package

* Wed Jan 29 2014 Jan Tulak <jan@tulak.me> - 1.0.2-1
- Added --verbose and --version flags and few minor changes in manual pages 
- and in spec file.

* Tue Jan 28 2014 Jan Tulak <jan@tulak.me> - 1.0.1-1
- Bugfixes, mainly in reading user inputs.

* Wed Dec 04 2013 Jan Tulak <jan@tulak.me> - 1.0.0-1
- Created.

