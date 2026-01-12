%global debug_package %{nil}

Name:           kamex
Version:        1.7.8
Release:        1%{?dist}
Summary:        High-performance SMS gateway (Kannel fork)
License:        MIT and Kannel
URL:            https://kamex.dev
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  openssl-devel
BuildRequires:  pkgconfig
BuildRequires:  mariadb-devel
BuildRequires:  libpq-devel
BuildRequires:  libsqlite3x-devel
BuildRequires:  hiredis-devel
BuildRequires:  gettext-devel
BuildRequires:  systemd-rpm-macros

Requires:       openssl
Recommends:     hiredis

%description
Kamex is a modern, high-performance SMS gateway with built-in admin panel.
Fork of Kannel with cleaned up codebase and active development.

Supports SMPP 3.3/3.4/5.0, EMI/UCP, HTTP, and GSM modems.

%package devel
Summary:        Development files for Kamex
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
Header files and libraries for developing applications that use Kamex.

%prep
%autosetup -n %{name}-%{version}

%build
autoreconf -fi
%configure \
    --enable-ssl \
    --with-mysql \
    --with-pgsql \
    --with-sqlite3 \
    --with-redis \
    --docdir=%{_docdir}/%{name}

%make_build

%install
%make_install

# Create directories
install -d %{buildroot}%{_sysconfdir}/kamex
install -d %{buildroot}%{_localstatedir}/log/kamex
install -d %{buildroot}%{_localstatedir}/spool/kamex
install -d %{buildroot}%{_unitdir}

# Install config
install -m 0640 doc/examples/kannel.conf %{buildroot}%{_sysconfdir}/kamex/kamex.conf

# Install systemd units
install -m 0644 contrib/systemd/kamex-bearerbox.service %{buildroot}%{_unitdir}/
install -m 0644 contrib/systemd/kamex-smsbox.service %{buildroot}%{_unitdir}/

# fakesmsc is now installed via bin_PROGRAMS in test/Makefile.am

# Remove static libraries and libtool files
rm -f %{buildroot}%{_libdir}/*.la
rm -f %{buildroot}%{_libdir}/*.a

%pre
getent group kamex >/dev/null || groupadd -r kamex
getent passwd kamex >/dev/null || \
    useradd -r -g kamex -d %{_localstatedir}/spool/kamex -s /sbin/nologin \
    -c "Kamex SMS Gateway" kamex
exit 0

%post
%systemd_post kamex-bearerbox.service kamex-smsbox.service

%preun
%systemd_preun kamex-bearerbox.service kamex-smsbox.service

%postun
%systemd_postun_with_restart kamex-bearerbox.service kamex-smsbox.service

%files
%license LICENSE LICENSE.kannel
%doc README.md CHANGELOG.md
%doc doc/
%dir %attr(0750, kamex, kamex) %{_sysconfdir}/kamex
%config(noreplace) %attr(0640, kamex, kamex) %{_sysconfdir}/kamex/kamex.conf
%{_sbindir}/bearerbox
%{_sbindir}/smsbox
%{_sbindir}/run_kannel_box
%{_bindir}/mtbatch
%{_bindir}/decode_emimsg
%{_bindir}/gw-config
%{_bindir}/fakesmsc
%{_mandir}/man1/mtbatch.1*
%{_unitdir}/kamex-bearerbox.service
%{_unitdir}/kamex-smsbox.service
%dir %attr(0750, kamex, kamex) %{_localstatedir}/log/kamex
%dir %attr(0750, kamex, kamex) %{_localstatedir}/spool/kamex
%{_libdir}/libgwlib.so.*
%{_libdir}/libgw.so.*

%files devel
%{_bindir}/gw-config
%{_includedir}/kamex/
%{_libdir}/libgwlib.so
%{_libdir}/libgw.so

%changelog
* Mon Jan 12 2026 Kamex Team <dev@kamex.dev> - 1.7.8-1
- Removed SQLite2 and libsdb database backends (obsolete)
- Cleaned up ~500 lines of dead code

* Mon Jan 12 2026 Kamex Team <dev@kamex.dev> - 1.7.6-1
- Fixed systemd service namespace compatibility
- Added RuntimeDirectory/StateDirectory/LogsDirectory
- Service paths now use @SBINDIR@ template

* Sun Jan 11 2026 Kamex Team <dev@kamex.dev> - 1.7.5-1
- Initial RPM package for EL10
- Fork of Kannel with modern codebase
- Removed legacy protocols (CIMD, OIS, SEMA, WAP)
- Added systemd service files
