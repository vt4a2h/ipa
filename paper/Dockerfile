FROM pandoc/latex:3.9.0.2-debian

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl git make python3 python3-venv \
    && rm -rf /var/lib/apt/lists/* \
    && tlmgr update --self \
    && tlmgr install ulem

ARG WG21_REF=4269f434ed688170d404be3324235768e82d5e77
RUN git clone https://github.com/mpark/wg21.git /opt/wg21 \
    && git -C /opt/wg21 checkout --detach "${WG21_REF}"

RUN ver="$(sed -n 's/^override PANDOC_VER := //p' /opt/wg21/base.mk)" \
    && pandoc --version | head -1 | grep -qF "pandoc ${ver}" \
       || { echo "base image pandoc ($(pandoc --version | head -1)) != framework pin (${ver}); update the FROM tag" >&2; exit 1; } \
    && mkdir -p "/opt/wg21/deps/pandoc/${ver}" \
    && ln -s "$(command -v pandoc)" "/opt/wg21/deps/pandoc/${ver}/pandoc"

RUN make -f /opt/wg21/base.mk /opt/wg21/deps/python

RUN echo "${WG21_REF}" > /opt/wg21/data/.wg21-ref \
    && cp -a /opt/wg21/data /opt/wg21/data.seed \
    && rm -rf /opt/wg21/data \
    && ln -s /work/.cache/wg21/data /opt/wg21/data

RUN mkdir -p /opt/wg21/run \
    && echo 'include /opt/wg21/flat.mk' > /opt/wg21/run/Makefile

RUN printf '%s\n' \
    '#!/bin/sh' \
    'mkdir -p /work/.cache/wg21/data /work/.cache/home /work/.cache/xdg /work/.cache/texmf-var' \
    'd=/opt/wg21/data; s=/opt/wg21/data.seed' \
    '[ "$(cat "$d/.wg21-ref" 2>/dev/null)" = "$(cat "$s/.wg21-ref")" ] || cp -R --preserve=mode,timestamps "$s/." "$d/"' \
    'if [ "$1" = update ]; then cd /opt/wg21/run && exec make update; fi' \
    'exec make -f /opt/wg21/flat.mk "$@"' \
    > /opt/wg21/entrypoint.sh \
    && chmod +x /opt/wg21/entrypoint.sh

ENV HOME=/work/.cache/home \
    XDG_CACHE_HOME=/work/.cache/xdg \
    TEXMFVAR=/work/.cache/texmf-var \
    TEXMFCACHE=/work/.cache/texmf-var

WORKDIR /work
ENTRYPOINT ["/opt/wg21/entrypoint.sh"]
CMD ["all"]
