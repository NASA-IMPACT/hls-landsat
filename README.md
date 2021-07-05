## hls-landsat
This repository contains the Dockerfiles for running the HLS landsat granule code on ECS.

The `hls-landsat` image uses [hls-base](https://github.com/NASA-IMPACT/hls-base/) as base image.

### Development
You will require an AWS profile which has ECR pull permissions for the base image.
```shell
$ docker build --no-cache -t hls-landsat-c2.
```

### CI
The repository contains two CI workflows. When commits are pushed to the dev branch a new image is built and pushed to ECR with no tag.

When a new release is created from master a new image is built and pushed to ECR with the release version as a tag.
