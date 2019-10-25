FROM 552819999234.dkr.ecr.us-east-1.amazonaws.com/espa/external:latest
ENV PREFIX=/usr/local \
    SRC_DIR=/usr/local/src \
    ESPAINC=/usr/local/include \
    ESPALIB=/usr/local/lib \
		L8_AUX_DIR=/usr/local/src \
    ECS_ENABLE_TASK_IAM_ROLE=true

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

RUN pip install gsutil
RUN pip install awscli
COPY lasrc_landsat_granule.sh ./usr/local/lasrc_landsat_granule.sh

ENTRYPOINT ["/bin/sh", "-c"]
# CMD ["/usr/local/bin/updatelads.py","--today"]
CMD ["/usr/local/lasrc_landsat_granule.sh"]
