# ------------------------------------------------------------------------------
# Builder
# ------------------------------------------------------------------------------

FROM docker.io/fedora:46@sha256:16fd7b21d98a67675452902cf5c2629aad746b66455f9b13c5af4b6d9799add9 AS builder

RUN dnf -y update && dnf -y install clang cmake curl git make python-sphinx

WORKDIR /app

RUN git clone https://git.jrpotter.com/jrpotter/makinori.git \
      --recurse-submodules  \
      --revision=8aca6f49e677eff13cad3bb216b8b20bb605d1b7 \
      .

RUN BUILD_TYPE=Release make docs

# ------------------------------------------------------------------------------
# Runtime
# ------------------------------------------------------------------------------

FROM docker.io/fedora:46@sha256:16fd7b21d98a67675452902cf5c2629aad746b66455f9b13c5af4b6d9799add9 AS runtime

RUN adduser --no-create-home --shell /sbin/nologin noroot

USER noroot

WORKDIR /app

COPY --chown=noroot:noroot --from=builder /app/build /app/build
COPY --chown=noroot:noroot --from=builder /app/docs /app/docs

EXPOSE 1314

ENTRYPOINT ["/app/build/bin/docs"]
