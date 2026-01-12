FROM registry.access.redhat.com/ubi10/ubi-minimal:latest

COPY kamex-*.rpm /tmp/

# Enable EPEL for hiredis (UBI doesn't have epel-release in repos)
RUN rpm -ivh https://dl.fedoraproject.org/pub/epel/epel-release-latest-10.noarch.rpm && \
    microdnf install -y openssl hiredis mariadb-connector-c libpq && \
    rpm -ivh /tmp/kamex-*.rpm && \
    rm -f /tmp/*.rpm && \
    microdnf clean all

RUN mkdir -p /var/log/kamex /var/spool/kamex /var/run/kamex && \
    chown -R kamex:kamex /var/log/kamex /var/spool/kamex /var/run/kamex

USER kamex
EXPOSE 13000 13013

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s \
    CMD curl -sf http://localhost:13000/health || exit 1

ENTRYPOINT ["/usr/sbin/bearerbox"]
CMD ["/etc/kamex/kamex.conf"]
