FROM centos:7.3.1611
RUN yum install -y \
        wget \
    && yum clean metadata
COPY download_lasrc_aux.sh ./usr/local/download_lasrc_aux.sh
COPY download_msilut.sh ./usr/local/download_msilut.sh
ENTRYPOINT ["/bin/sh", "-c"]
CMD ["./usr/local/download_lasrc_aux.sh"]
