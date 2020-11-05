ARG AWS_ACCOUNT_ID
FROM ${AWS_ACCOUNT_ID}.dkr.ecr.us-west-2.amazonaws.com/hls-base:latest
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
    PYTHONPATH="${PREFIX}/lib/python2.7/site-packages" \
    ACCODE=LaSRCL8V3.5.5

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

# RUN pip install --upgrade git+https://github.com/NASA-IMPACT/espa-python-library.git@v1.0-hls

COPY ./python_scripts/* ${PREFIX}/bin/
COPY ./scripts/* ${PREFIX}/bin/

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["landsat.sh"]
