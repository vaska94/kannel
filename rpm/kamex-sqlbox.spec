%global debug_package %{nil}

Name:           kamex-sqlbox
Version:        1.7.7
Release:        1%{?dist}
Summary:        Database queue box for Kamex SMS gateway
License:        MIT and Kannel
URL:            https://kamex.dev
Source0:        kamex-%{version}.tar.gz

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
BuildRequires:  systemd-rpm-macros
BuildRequires:  kamex-devel = %{version}

Requires:       kamex = %{version}

%description
SQLBox is a special Kamex box that sits between bearerbox and smsbox
and uses a database queue to store and forward messages.

Messages are queued on a configurable table (defaults to send_sms) and
moved to another table (defaults to sent_sms) afterwards.

You can also manually insert messages into the send_sms table and they
will be sent and moved to the sent_sms table as well. This allows for
fast and easy injection of large amounts of messages into Kamex.

%prep
%autosetup -n kamex-%{version}

%build
cd addons/sqlbox
autoreconf -fi
%configure \
    --with-kannel-dir=%{_prefix}
%make_build

%install
cd addons/sqlbox
%make_install

# Create directories
install -d %{buildroot}%{_sysconfdir}/kamex
install -d %{buildroot}%{_unitdir}

# Install config
install -m 0640 example/sqlbox.conf.example %{buildroot}%{_sysconfdir}/kamex/sqlbox.conf

# Generate and install systemd unit from template
sed 's|@SBINDIR@|%{_sbindir}|g' ../../contrib/systemd/kamex-sqlbox.service.in > kamex-sqlbox.service
install -m 0644 kamex-sqlbox.service %{buildroot}%{_unitdir}/

%pre
# Ensure kamex user exists (should be created by kamex package)
getent group kamex >/dev/null || groupadd -r kamex
getent passwd kamex >/dev/null || \
    useradd -r -g kamex -d %{_localstatedir}/spool/kamex -s /sbin/nologin \
    -c "Kamex SMS Gateway" kamex
exit 0

%post
%systemd_post kamex-sqlbox.service

%preun
%systemd_preun kamex-sqlbox.service

%postun
%systemd_postun_with_restart kamex-sqlbox.service

%files
%license addons/sqlbox/COPYING addons/sqlbox/KannelLICENSE
%doc addons/sqlbox/README addons/sqlbox/ChangeLog addons/sqlbox/AUTHORS
%config(noreplace) %attr(0640, kamex, kamex) %{_sysconfdir}/kamex/sqlbox.conf
%{_sbindir}/sqlbox
%{_unitdir}/kamex-sqlbox.service

%changelog
* Mon Jan 12 2026 Kamex Team <dev@kamex.dev> - 1.7.7-1
- Removed SQLite2 and libsdb database backends (obsolete)

* Mon Jan 12 2026 Kamex Team <dev@kamex.dev> - 1.7.6-1
- Initial RPM package for EL10
- Modernized configure.ac, removed DocBook
- Added systemd service file with security hardening
