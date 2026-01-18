# Pin to specific digest for reproducible builds
# Update: skopeo inspect docker://registry.access.redhat.com/ubi10/ubi-minimal:latest
FROM registry.access.redhat.com/ubi10/ubi-minimal@sha256:67aafc6c9c44374e1baf340110d4c835457d59a0444c068ba9ac6431a6d9e7ac

# For reproducible builds, pass SOURCE_DATE_EPOCH:
#   docker build --build-arg SOURCE_DATE_EPOCH=$(git log -1 --format=%ct) .
ARG SOURCE_DATE_EPOCH
ENV SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH}

COPY kamex-*.rpm /tmp/

# Enable EPEL for hiredis - pin version for reproducibility
RUN rpm -ivh https://dl.fedoraproject.org/pub/epel/10/Everything/x86_64/Packages/e/epel-release-10-7.el10_1.noarch.rpm && \
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
