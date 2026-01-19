%global debug_package %{nil}

Name:           kamex-opensmppbox
Version:        1.8.3
Release:        1%{?dist}
Summary:        SMPP proxy box for Kamex SMS gateway
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
BuildRequires:  systemd-rpm-macros
BuildRequires:  kamex-devel = %{version}

Requires:       kamex = %{version}

%description
OpenSMPPBox is a special Kamex box that acts as an SMPP server/proxy.
It accepts SMPP connections from external clients and forwards messages
to bearerbox.

This allows external applications to connect via SMPP protocol instead
of using the HTTP API, useful for integrating with existing SMPP-based
systems.

%prep
%autosetup -n kamex-%{version}

%build
cd addons/opensmppbox
autoreconf -fi
%configure \
    --with-kannel-dir=%{_prefix}
%make_build

%install
cd addons/opensmppbox
%make_install

# Create directories
install -d %{buildroot}%{_sysconfdir}/kamex
install -d %{buildroot}%{_unitdir}

# Install config
install -m 0640 example/opensmppbox.conf.example %{buildroot}%{_sysconfdir}/kamex/opensmppbox.conf
install -m 0640 example/smpplogins.txt.example %{buildroot}%{_sysconfdir}/kamex/smpplogins.txt

# Generate and install systemd unit from template
sed 's|@SBINDIR@|%{_sbindir}|g' ../../contrib/systemd/kamex-opensmppbox.service.in > kamex-opensmppbox.service
install -m 0644 kamex-opensmppbox.service %{buildroot}%{_unitdir}/

%pre
# Ensure kamex user exists (should be created by kamex package)
getent group kamex >/dev/null || groupadd -r kamex
getent passwd kamex >/dev/null || \
    useradd -r -g kamex -d %{_localstatedir}/spool/kamex -s /sbin/nologin \
    -c "Kamex SMS Gateway" kamex
exit 0

%post
%systemd_post kamex-opensmppbox.service

%preun
%systemd_preun kamex-opensmppbox.service

%postun
%systemd_postun_with_restart kamex-opensmppbox.service

%files
%license addons/opensmppbox/COPYING addons/opensmppbox/LICENSE.kannel
%doc addons/opensmppbox/README addons/opensmppbox/ChangeLog addons/opensmppbox/AUTHORS
%config(noreplace) %attr(0640, kamex, kamex) %{_sysconfdir}/kamex/opensmppbox.conf
%config(noreplace) %attr(0640, kamex, kamex) %{_sysconfdir}/kamex/smpplogins.txt
%{_sbindir}/opensmppbox
%{_unitdir}/kamex-opensmppbox.service

%changelog
* Mon Jan 12 2026 Kamex Team <dev@kamex.dev> - 1.7.8-1
- Initial RPM package for EL10
- Modernized configure.ac, removed DocBook
- Added systemd service file with security hardening
