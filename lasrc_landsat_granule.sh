#!/bin/bash

# Exit on any error
set -o errexit

# id="LC08_L1TP_027039_20190901_20190901_01_RT"
id=$LANDSAT_SCENE_ID
landsatdir=/tmp/${id}

# Remove tmp files on exit
trap "rm -rf $landsatdir; exit" INT TERM EXIT

bucket=$OUTPUT_BUCKET
IFS='_'
# Read into an array as tokens separated by IFS
read -ra ADDR <<< "$id"

# Format GCS url and download
url=gs://gcp-public-data-landsat/LC08/01/${ADDR[2]:0:3}/${ADDR[2]:3:5}/${id}/
gsutil -m cp -r "$url" /tmp

# Enter working directory
cd "$landsatdir"

# Run Fmask
fmask_usgsLandsatStacked.py -o fmask.img --scenedir ./

# Convert to flat binary
gdal_translate -of ENVI fmask.img fmask.bin

# Convert data from tiled to scanline for espa formatting
for f in *.TIF
  do
  gdal_translate -co TILED=NO "$f" "${f}_scan"
  rm "$f"
  mv "${f}_scan" "$f"
  done

mtl=${id}_MTL.txt
espa_xml="${id}.xml"
hls_espa_xml="${id}_hls.xml"
srhdf=${id}_sr.hdf

# Convert to espa format
convert_lpgs_to_espa --mtl="$mtl"

# Run lasrc
do_lasrc_l8.py --xml "$espa_xml"

# Create ESPA xml file using HLS v1.5 band names
alter_sr_band_names.py -i "$espa_xml" -o "$hls_espa_xml"

# Convert ESPA xml file to HDF
convert_espa_to_hdf --xml="$hls_espa_xml" --hdf="$srhdf"

aws s3 sync . "s3://${bucket}/${id}/"
