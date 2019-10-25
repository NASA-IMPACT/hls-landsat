#!/bin/bash
# id="LC08_L1TP_027039_20190901_20190901_01_RT"
id=$LANDSAT_SCENE_ID
bucket=$OUTPUT_BUCKET
IFS='_'
# Read into an array as tokens separated by IFS
read -ra ADDR <<< "$id"

# Format GCS url and download
url=gs://gcp-public-data-landsat/LC08/01/${ADDR[2]:0:3}/${ADDR[2]:3:5}/${id}/
gsutil -m cp -r "$url" /tmp
landsatdir=/tmp/${id}
cd "$landsatdir"

# Convert data from tiled to scanline for espa formatting
for f in *.TIF
	do
	gdal_translate -co TILED=NO "$f" "${f}_scan"
	rm "$f"
	mv "${f}_scan" "$f"
	done
mtl=${id}_MTL.txt

# Convert to espa format
convert_lpgs_to_espa --mtl "$mtl"

# Run lasrc
do_lasrc_l8.py --xml "${id}.xml"
if [ $? -ne 0 ]
then
  echo "Lasrc failed"
else
  convert_espa_to_hdf --xml "${id}.xml" --hdf  "${id}_sr.hdf"
  # aws s3api put-object --bucket "$bucket" --key "$id"
  aws s3 sync . "s3://${bucket}/"
fi
