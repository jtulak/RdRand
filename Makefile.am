## LibRdRand:
## an rdrand of using Automake and Libtool to build a shared library

## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

################
bin_PROGRAMS = rdrand-gen

rdrand_gen_SOURCES = src/rdrand-gen.c
rdrand_gen_LDADD = src/librdrand.c 
rdrand_gen_LDFLAGS = @OPENMP_CFLAGS@ @RDRND_FLAGS@  -lrt -lm
rdrand_gen_CFLAGS = @OPENMP_CFLAGS@ @CFLAGS@

##########

man_MANS = man/rdrand-gen.7
notrans_man_MANS = man/rdrand-lib.3

##########


## Define a libtool archive target "librdrand-@RDRAND_API_VERSION@.la", with
## @RDRAND_API_VERSION@ substituted into the generated Makefile at configure
## time.
## The libtool archive file (.la) will be installed into the directory named
## by the predefined variable $(bindir), along with the actual shared library
## file (.so).
# lib_LTLIBRARIES = librdrand-@RDRAND_API_VERSION@.la
lib_LTLIBRARIES = librdrand.la

## Define the source file list for the "librdrand-@RDRAND_API_VERSION@.la"
## target.  Note that @RDRAND_API_VERSION@ is not interpreted by Automake and
## will therefore be treated as if it were literally part of the target name,
## and the variable name derived from that.
## The file extension .cc is recognized by Automake, and makes it produce
## rules which invoke the C++ compiler to produce a libtool object file (.lo)
## from each source file.  Note that it is not necessary to list header files
## which are already listed elsewhere in a _HEADERS variable assignment.
librdrand_la_SOURCES = src/librdrand.c

## Instruct libtool to include ABI version information in the generated shared
## library file (.so).  The library ABI version is defined in configure.ac, so
## that all version information is kept in one place.
librdrand_la_LDFLAGS = -version-info $(RDRAND_SO_VERSION)

## Define the list of public header files and their install location.  The
## nobase_ prefix instructs Automake to not strip the directory part from each
## filename, in order to avoid the need to define separate file lists for each
## installation directory.  This only works if the directory hierarchy in the
## source tree matches the hierarchy at the install location, however.
# rdrand_includedir = $(includedir)/rdrand-$(RDRAND_API_VERSION)
rdrand_includedir = $(includedir)/
# nobase_rdrand_include_HEADERS = rdrand.h
nobase_rdrand_include_HEADERS = librdrand.h

## The generated configuration header is installed in its own subdirectory of
## $(libdir).  The reason for this is that the configuration information put
## into this header file describes the target platform the installed library
## has been built for.  Thus the file must not be installed into a location
## intended for architecture-independent files, as defined by the Filesystem
## Hierarchy Standard (FHS).
## The nodist_ prefix instructs Automake to not generate rules for including
## the listed files in the distribution on 'make dist'.  Files that are listed
## in _HEADERS variables are normally included in the distribution, but the
## configuration header file is generated at configure time and should not be
## shipped with the source tarball.
# rdrand_libincludedir = $(libdir)/rdrand-$(RDRAND_API_VERSION)/include
rdrand_libincludedir = $(libdir)/librdrand/include
nodist_rdrand_libinclude_HEADERS = rdrandconfig.h

## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information.  Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.
pkgconfigdir = $(libdir)/pkgconfig
# pkgconfig_DATA = rdrand-$(RDRAND_API_VERSION).pc
pkgconfig_DATA = librdrand.pc

## Define an independent executable script for inclusion in the distribution
## archive.  However, it will not be installed on an end user's system due to
## the noinst_ prefix.
dist_noinst_SCRIPTS = autogen.sh
