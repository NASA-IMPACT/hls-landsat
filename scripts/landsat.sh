#!/bin/bash

# Exit on any error
set -o errexit

id="$LANDSAT_SCENE_ID"
landsatdir="/tmp/${id}"
fmask="${id}_Fmask4.tif"
fmaskbin=fmask.bin

# Remove tmp files on exit
trap "rm -rf $landsatdir; exit" INT TERM EXIT

bucket="$OUTPUT_BUCKET"
IFS='_'

# Read into an array as tokens separated by IFS
read -ra ADDR <<< "$id"

# Format GCS url and download
url="gs://gcp-public-data-landsat/LC08/01/${ADDR[2]:0:3}/${ADDR[2]:3:5}/${id}/"
gsutil -m cp -r "$url" /tmp

# Enter working directory
cd "$landsatdir"

# Run Fmask
# hlsfmask_usgsLandsatStacked.py -o fmask.img --strict --scenedir ./
/usr/local/MATLAB/application/run_Fmask_4_0.sh /usr/local/MATLAB/v95

# Convert to flat binary
gdal_translate -of ENVI "$fmask" "$fmaskbin"

# Convert data from tiled to scanline for espa formatting
for f in *.TIF
  do
  gdal_translate -co TILED=NO "$f" "${f}_scan.tif"
  rm "$f"
  mv "${f}_scan.tif" "$f"
  rm "${f}_scan.IMD"
  done

mtl="${id}_MTL.txt"
espa_xml="${id}.xml"
hls_espa_xml="${id}_hls.xml"
srhdf="${id}_sr.hdf"
outputhdf="${id}_out.hdf"

# Convert to espa format
convert_lpgs_to_espa --mtl="$mtl"

# Run lasrc
do_lasrc_landsat.py --xml "$espa_xml"

# Create ESPA xml file using HLS v1.5 band names
alter_sr_band_names.py -i "$espa_xml" -o "$hls_espa_xml"

# Convert ESPA xml file to HDF
convert_espa_to_hdf --xml="$hls_espa_xml" --hdf="$srhdf"

# Run addFmaskSDS
addFmaskSDS "$srhdf" "$fmaskbin" "$mtl" "LaSRC" "$outputhdf" >&2

# Copy files to S3
# aws s3 sync . "s3://${bucket}/${id}/"

aws s3 cp "${landsatdir}/${outputhdf}" "s3://${bucket}/${id}/${outputhdf}"
