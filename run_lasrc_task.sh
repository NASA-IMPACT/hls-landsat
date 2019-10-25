#!/bin/bash
stack=$1
cluster=$(aws cloudformation describe-stacks --stack-name "$stack" \
--query "Stacks[0].Outputs[?OutputKey=='ClusterName'].OutputValue" --output text)
task=$(aws cloudformation describe-stacks --stack-name "$stack" \
--query "Stacks[0].Outputs[?OutputKey=='LaSRCTaskDefinitionArn'].OutputValue" --output text)
granuletype=LANDSAT_SCENE_ID
command=/usr/local/lasrc_landsat_granule.sh
# granuleid=LC08_L1TP_027039_20190901_20190901_01_RT
granuleid=$2
overrides=$(cat <<EOF
{
  "containerOverrides": [
    {
      "name": "lasrc",
      "command": ["$command"],
      "environment": [
        {
          "name": "$granuletype",
          "value": "$granuleid"
        },
        {
          "name": "LASRC_AUX_DIR",
          "value": "/var/lasrc_aux"
        }
      ]
    }
  ]
}
EOF
)
echo "$overrides" > ./overrides.json
aws ecs run-task --overrides file://overrides.json --task-definition "$task" \
  --cluster "$cluster"
rm ./overrides.json
