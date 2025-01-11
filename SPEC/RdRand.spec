Summary:        Library for generating random numbers using the RDRAND (read random) instruction
Name:           RdRand
Version:        2.1.6
Release:        1%{?dist}
# Automatically converted from old format: LGPLv2+ - review is highly recommended.
License:        LGPLv2+
URL:            https://github.com/jirka-h/%{name}
Source0:        https://github.com/jirka-h/%{name}/archive/%{version}.tar.gz
ExclusiveArch: %{ix86} x86_64
Requires:       openssl
BuildRequires: make libtool
BuildRequires:  gcc-c++
BuildRequires:  openssl-devel
%description
RdRand is an instruction for returning random numbers from an Intel on-chip
hardware random number generator.RdRand is available in Ivy Bridge and later
processors.

It uses cascade construction, combining a HW RNG operating at 3Gbps with CSPRNG
with all components sealed on CPU. The entropy source is a meta-stable circuit,
with unpredictable behavior based on thermal noise. The entropy is fed into
a 3:1 compression ratio entropy extractor (whitener) based on AES-CBC-MAC.
Online statistical tests are performed at this stage and only high quality
random data are used as the seed for cryptographically secure SP800-90 AES-CTR
DRBG compliant PRNG.
This generator is producing maximum of 512 128-bit AES blocks before it's
reseeded. According to documentation the 512 blocks is a upper limit for
reseed, in practice it reseeds much more frequently.

%package devel
Summary:        Development files for the RdRand
Requires:       %{name}%{?_isa} = %{version}-%{release}, openssl-devel

%description devel
Headers and shared object symbolic links for the RdRand.


%prep
%setup -q
autoreconf -fi

%build
%configure
make %{?_smp_mflags}

%install
make install DESTDIR=$RPM_BUILD_ROOT INSTALL="%{__install} -p"
rm -vf $RPM_BUILD_ROOT{%{_libdir}/librdrand.la,%{_libdir}/librdrand.a,%{_libdir}/librdrand/include/rdrandconfig.h}

%ldconfig_scriptlets

%files
%doc README COPYING ChangeLog NEWS
%{_bindir}/rdrand-gen
%{_mandir}/man7/rdrand-gen.7*
%{_libdir}/librdrand.so.*

%files devel
%{_mandir}/man3/librdrand.3*
%{_includedir}/librdrand.h
%{_includedir}/librdrand-aes.h
%{_libdir}/librdrand.so
%{_libdir}/pkgconfig/*

%changelog
* Sat Jan 11 2025  Jirka Hladky <hladky.jiri@gmail.com> - 2.1.6-1
- Updated to v2.1.6
- Fixes https://bugzilla.redhat.com/show_bug.cgi?id=2336261

* Wed Sep 04 2024 Miroslav Suchý <msuchy@redhat.com> - 2.1.4-8
- convert license to SPDX

* Wed Jul 17 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.4-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_41_Mass_Rebuild

* Mon Jan 22 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.4-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_40_Mass_Rebuild

* Fri Jan 19 2024 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.4-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_40_Mass_Rebuild

* Wed Jul 19 2023 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.4-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_39_Mass_Rebuild

* Wed Jan 18 2023 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.4-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_38_Mass_Rebuild

* Wed Jul 20 2022 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.4-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_37_Mass_Rebuild

* Thu Feb 17 2022  Jirka Hladky <hladky.jiri@gmail.com> - 2.1.4-1
- Updated to v2.1.4

* Wed Jan 19 2022 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.2-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_36_Mass_Rebuild

* Tue Sep 14 2021 Sahana Prasad <sahana@redhat.com> - 2.1.2-6
- Rebuilt with OpenSSL 3.0.0

* Wed Jul 21 2021 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.2-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_35_Mass_Rebuild

* Mon Jan 25 2021 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.2-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_34_Mass_Rebuild

* Mon Jul 27 2020 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Tue Jun 16 2020 Jirka Hladky <hladky.jiri@gmail.com> - 2.1.2-2
- Fixed typo in rpm spec file

* Tue Jun 16 2020 Jirka Hladky <hladky.jiri@gmail.com> - 2.1.2-1
- Added support for AMD CPUs

* Tue Jan 28 2020 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.1-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Wed Jul 24 2019 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.1-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Thu Jan 31 2019 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.1-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Thu Jul 12 2018 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.1-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Fri Feb 09 2018 Igor Gnatenko <ignatenkobrain@fedoraproject.org> - 2.1.1-5
- Escape macros in %%changelog

* Wed Feb 07 2018 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.1-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Wed Aug 02 2017 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.1-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Binutils_Mass_Rebuild

* Wed Jul 26 2017 Fedora Release Engineering <releng@fedoraproject.org> - 2.1.1-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Tue Feb 28 2017 Jan Tulak <jan@tulak.me> - 2.1.1-1
- Fix the output of --version for current version

* Mon Feb 20 2017 Jan Tulak <jan@tulak.me> - 2.1.0-1
- Update for OpenSSL 1.1.0
- Remove an option that wasn't implemented

* Fri Feb 10 2017 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.0-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Wed Feb 03 2016 Fedora Release Engineering <releng@fedoraproject.org> - 2.0.0-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Tue Jun 16 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.0.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Fri Aug 15 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.0.0-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Fri Jun 06 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.0.0-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Sat May 24 2014 Jan Tulak <jan@tulak.me> - 2.0.0-1
- Partial rewriting, added AES encryption of generated values.

* Mon Feb 17 2014 Jan Tulak <jan@tulak.me> - 1.0.5-1
- Fixed bug with parsing -t argument in rdrand-gen.

* Thu Feb 6 2014 Jan Tulak <jan@tulak.me> - 1.0.4-2
- Removed %%defattr from devel subpackage, removed commented out lines,
- man page suffix changed from .gz to .*

* Wed Feb 5 2014 Jan Tulak <jan@tulak.me> - 1.0.4-1
- ExclusiveArch, removed %%clean, %%defattr, added blank lines between changelogs.
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

