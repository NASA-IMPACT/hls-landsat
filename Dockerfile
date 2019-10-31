FROM 552819999234.dkr.ecr.us-east-1.amazonaws.com/hls-base:latest
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
		L8_AUX_DIR=/usr/local/src \
    ECS_ENABLE_TASK_IAM_ROLE=true \
    PYTHONPATH="${PREFIX}/lib/python2.7/site-packages"

# Move and compile addFmaskSDS
COPY ./hls_libs/addFmaskSDS ${SRC_DIR}/addFmaskSDS
RUN cd ${SRC_DIR}/addFmaskSDS \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf addFmaskSDS

COPY lasrc_landsat_granule.sh ./usr/local/lasrc_landsat_granule.sh

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["/usr/local/lasrc_landsat_granule.sh"]
