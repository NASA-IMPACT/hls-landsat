#!/bin/bash
stack=$1
cluster=$(aws cloudformation describe-stacks --stack-name "$stack" \
--query "Stacks[0].Outputs[?OutputKey=='ClusterName'].OutputValue" --output text)
task=$(aws cloudformation describe-stacks --stack-name "$stack" \
--query "Stacks[0].Outputs[?OutputKey=='LaSRCTaskDefinitionArn'].OutputValue" --output text)
overrides=$(cat <<EOF
{
  "containerOverrides": [
    {
      "name": "lasrc",
      "command": ["/usr/local/bin/updatelads.py --today"],
      "environment": [
        {
          "name": "L8_AUX_DIR",
          "value": "/var/lasrc_aux"
        },
        {
          "name": "LAADS_TOKEN",
          "value": "$LAADS_TOKEN" 
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
