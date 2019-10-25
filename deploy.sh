#!/bin/bash
# read -p 'Stack name: ' stackname
aws cloudformation deploy --template-file cloudformation.yaml --stack-name \
lasrc --tags Project=hls --region us-east-1 --capabilities CAPABILITY_IAM
