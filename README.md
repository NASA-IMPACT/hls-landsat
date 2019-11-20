## hls-landsat
This repository contains the Dockerfiles for running the HLS landsat granule code on ECS.

The `hls-landsat` image uses [hls-base](https://github.com/NASA-IMPACT/hls-base/) as base image.

After building your base dependencies image and pushing it to ECR you can build the `hls-landsat` processing image with

```shell
$ docker build --tag hls-landsat .
```
You can then tag this `hls-landsat` image as `350996086543.dkr.ecr.us-west-2.amazonaws.com/hls-landsat` and push it to ECR.
