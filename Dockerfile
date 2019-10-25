FROM 552819999234.dkr.ecr.us-east-1.amazonaws.com/espa/external:latest
ENV PREFIX=/usr/local \
    SRC_DIR=/usr/local/src \
    ESPAINC=/usr/local/include \
    ESPALIB=/usr/local/lib \
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
    PYTHONPATH="${PYTHONPATH}:${PREFIX}/lib/python2.7/site-packages"

RUN REPO_NAME=espa-product-formatter \
    # && REPO_TAG=product_formatter_v1.16.1 \
    && cd $SRC_DIR \
    && git clone https://github.com/USGS-EROS/${REPO_NAME}.git ${REPO_NAME} \
    && cd ${REPO_NAME} \
    # && git checkout ${REPO_TAG} \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes \
    && make install \
    && cd $SRC_DIR \
    && rm -rf ${REPO_NAME}

RUN REPO_NAME=espa-surface-reflectance \
    && cd $SRC_DIR \
    && git clone https://github.com/developmentseed/${REPO_NAME}.git \
    && cd ${REPO_NAME} \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes \
    && make install \
    && make install-lasrc-aux \
    && cd $SRC_DIR \
    && rm -rf ${REPO_NAME}

# Move and compile addFmaskSDS
COPY ./hls_libs/addFmaskSDS ${SRC_DIR}/addFmaskSDS
RUN cd ${SRC_DIR}/addFmaskSDS \
    && make BUILD_STATIC=yes ENABLE_THREADING=yes \
    && make clean \
    && make install \
    && cd $SRC_DIR \
    && rm -rf addFmaskSDS

RUN pip install scipy

RUN cd ${SRC_DIR} \
    && wget https://bitbucket.org/chchrsc/rios/downloads/rios-1.4.8.tar.gz \
    && tar xfv rios-1.4.8.tar.gz \
    && cd rios-1.4.8 \
    && python setup.py install --prefix=${PREFIX}

RUN cd ${SRC_DIR} \
    && wget https://bitbucket.org/chchrsc/python-fmask/downloads/python-fmask-0.5.4.tar.gz \
    && tar xfz python-fmask-0.5.4.tar.gz \
    && cd python-fmask-0.5.4 \
    && python setup.py build \
    && python setup.py install

RUN pip install gsutil
RUN pip install awscli
COPY lasrc_landsat_granule.sh ./usr/local/lasrc_landsat_granule.sh

ENTRYPOINT ["/bin/sh", "-c"]
# CMD ["/usr/local/bin/updatelads.py","--today"]
# CMD ["/usr/local/lasrc_landsat_granule.sh"]
