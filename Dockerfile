FROM 018923174646.dkr.ecr.us-west-2.amazonaws.com/hls-base-3.0.5
ENV PREFIX=/usr/local \
    SRC_DIR=/usr/local/src \
    GCTPLIB=/usr/local/lib \
    HDFLIB=/usr/local/lib \
    ZLIB=/usr/local/lib \
    SZLIB=/usr/local/lib \
    JPGLIB=/usr/local/lib \
    PROJLIB=/usr/local/lib \
    HDFINC=/usr/local/include \
    GCTPINC=/usr/local/include \
    GCTPLINK="-lGctp -lm" \
    HDFLINK=" -lmfhdf -ldf -lm" \
    ECS_ENABLE_TASK_IAM_ROLE=true \
    ACCODE=LaSRCL8V3.0.5 \
    PYTHONPATH="${PYTHONPATH}:${PREFIX}/lib/python3.6/site-packages" \
    ACCODE=LaSRCL8V3.5.5 \
    LC_ALL=en_US.utf-8 \
    LANG=en_US.utf-8

# The Python click library requires a set locale
RUN localedef -i en_US -f UTF-8 en_US.UTF-8

# Move common files to source directory
COPY ./hls_libs/common $SRC_DIR

# Move and compile addFmaskSDS
COPY ./hls_libs/addFmaskSDS ${SRC_DIR}/addFmaskSDS
RUN cd ${SRC_DIR}/addFmaskSDS \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf addFmaskSDS

RUN pip3 install git+https://github.com/NASA-IMPACT/hls-utilities@v1.6

COPY ./scripts/* ${PREFIX}/bin/
ENTRYPOINT ["/bin/sh", "-c"]
CMD ["landsat.sh"]
