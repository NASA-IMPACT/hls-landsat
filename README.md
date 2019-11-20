## hls-landsat
This repository contains cloudformation templates and Dockerfiles for running the HLS landsat granule code on ECS.

The `lasrc` and `hls` code requires a number of [dependencies](https://github.com/developmentseed/espa-surface-reflectance/tree/master/lasrc#dependencies). To manage these dependencies in a more streamlined way the `Dockerfile` uses a base image which can be built using the `usgs.espa.centos.external` template defined in the [espa-dockerfiles](https://github.com/developmentseed/espa-dockerfiles) repository.
See the instructions in the [espa-dockerfiles](https://github.com/developmentseed/espa-dockerfiles) repository for building the external dependencies image.

After building the dependencies image, following the steps outlined [here]https://docs.aws.amazon.com/AmazonECR/latest/userguide/ECR_AWSCLI.html) you can tag this image as `552819999234.dkr.ecr.us-east-1.amazonaws.com/espa/external` and push it to ECR.

After building your base dependencies image and pushing it to ECR you can build the `hls-landsat` processing image with
```shell
$ docker build --tag hls-landsat .
```
You can then tag this `hls-landsat` image as `552819999234.dkr.ecr.us-east-1.amazonaws.com/hls-landsat` and push it to ECR.

Now that we have the necessary images in ECR we can build the AWS infrastructure to run these containers as tasks. The included Cloudformation template contains all the necessary stack resources, to deploy a new stack run

```shell
$ ./deploy.sh
```
You will be prompted to enter a stack name.  This script assumes you have [configured](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-files.html) your AWS credentials and that your account has sufficient permissions to create new resources.  The remainder of the README will reference this stack name as `yourstackname`.

The `lasrc` portion of the `hls` processing code requires [auxiliary data](https://github.com/developmentseed/espa-surface-reflectance/tree/master/lasrc#downloads) to run.
This `lasrc` auxiliary data also requires [periodic updates](https://github.com/developmentseed/espa-surface-reflectance/tree/master/lasrc#auxiliary-data-updates) to run.

The task definitions for updating this `lasrc` auxiliary data are included in `./cloudformation.yaml' but see the [lasrc-auxiliary](https://github.com/developmentseed/lasrc-auxiliary) repo for more details on creating the images and running the tasks on ECS.

Once the auxiliary data is updated, to run the hls-landsat code as a task for a Landsat granule run
```shell
$ ./run_hlslandsat_task.sh yourstackname LC08_L1TP_170078_20190303_20190309_01_T1
```

