#!/bin/bash

# Exit on any error
set -o errexit

jobid="$AWS_BATCH_JOB_ID"
# shellcheck disable=2153
granule="$GRANULE"
bucket="$OUTPUT_BUCKET"
inputbucket="$INPUT_BUCKET"
# shellcheck disable=2153
prefix="$PREFIX"
inputgranule="s3://${inputbucket}/${prefix}"
workingdir="/var/scratch/${jobid}"
granuledir="${workingdir}/${granule}"
# shellcheck disable=2153
debug_bucket="$DEBUG_BUCKET"
# shellcheck disable=2153
ACCODE=Lasrc

# Remove tmp files on exit
# shellcheck disable=2064
trap "rm -rf $workingdir; exit" INT TERM EXIT

rename_angle_bands () {
  anglebasename=$1
  newbasename=$2
  mv "${anglebasename}_vaa.hdr" "${newbasename}_VAA.hdr"
  mv "${anglebasename}_vaa.img" "${newbasename}_VAA.img"
  mv "${anglebasename}_vza.hdr" "${newbasename}_VZA.hdr"
  mv "${anglebasename}_vza.img" "${newbasename}_VZA.img"
  mv "${anglebasename}_saa.hdr" "${newbasename}_SAA.hdr"
  mv "${anglebasename}_saa.img" "${newbasename}_SAA.img"
  mv "${anglebasename}_sza.hdr" "${newbasename}_SZA.hdr"
  mv "${anglebasename}_sza.img" "${newbasename}_SZA.img"
}

# Create workingdir
mkdir -p "$granuledir"

fmask="${granule}_Fmask4.tif"
fmaskbin=fmask.bin

echo "Start processing granules"

echo "Copying granule from USGS S3"
aws s3 cp "$inputgranule" "$granuledir" --recursive --request-payer requester

IFS='_'
read -ra granulecomponents <<< "$granule"
date=${granulecomponents[3]:0:8}
year=${date:0:4}
month=${date:4:2}
day=${date:6:2}
pathrow=${granulecomponents[2]}
outputname="${year}-${month}-${day}_${pathrow}"

# Check solar zenith angle.
echo "Check solar azimuth"
mtl="${granuledir}/${granule}_MTL.txt"
solar_zenith_valid=$(check_solar_zenith_landsat "$mtl")
if [ "$solar_zenith_valid" == "invalid" ]; then
  echo "Invalid solar zenith angle. Exiting now"
  exit 3
fi

# Enter working directory
cd "$granuledir"

# ovr and IMD files in AWS PDS break Fmask
# rm *.ovr
# rm *.IMD

# Run Fmask
/usr/local/MATLAB/application/run_Fmask_4_3.sh /usr/local/MATLAB/v96

# Convert to flat binary
gdal_translate -of ENVI "$fmask" "$fmaskbin"

# Convert data from tiled to scanline for espa formatting
echo "Convert to scanline"
for f in *.TIF
  do
  gdal_translate -co TILED=NO "$f" "${f}_scan.tif"
  rm "$f"
  mv "${f}_scan.tif" "$f"
  done

espa_xml="${granule}.xml"
hls_espa_xml="${granule}_hls.xml"
srhdf="sr.hdf"
outputhdf="${outputname}.hdf"

# Convert to espa format
echo "Convert to ESPA"
convert_lpgs_to_espa --mtl="$mtl"

# Run lasrc
echo "Run lasrc"
do_lasrc_landsat.py --xml "$espa_xml"

# Rename Angle bands to align with Collection 2 naming.
echo "Rename angle bands"
rename_angle_bands "${granule}" "$outputname"

# Create ESPA xml file using HLS v1.5 band names
echo "Create updated espa xml"
create_landsat_sr_hdf_xml "$espa_xml" "$hls_espa_xml"

# Convert ESPA xml file to HDF
echo "Convert to HDF"
convert_espa_to_hdf --xml="$hls_espa_xml" --hdf="$srhdf"

# Run addFmaskSDS
echo "Run addFmaskSDS"
aerosol_qa="${granule}_sr_aerosol.img"
echo "$aerosol_qa"
addFmaskSDS "$srhdf" "$fmaskbin" "$aerosol_qa" "$mtl" "$ACCODE" "$outputhdf"

if [ -z "$debug_bucket" ]; then
  aws s3 cp "${outputhdf}" "s3://${bucket}/${outputname}.hdf"
  aws s3 cp "$granuledir" "s3://${bucket}" --exclude "*" --include "*_VAA.img" \
    --include "*_VAA.hdr" --include "*_VZA.hdr" --include "*_VZA.img" \
    --include "*_SAA.hdr" --include "*_SAA.img" --include "*_SZA.hdr" \
    --include "*_SZA.img" --recursive --quiet
else
  # Copy all intermediate files to debug bucket.
  timestamp=$(date +'%Y_%m_%d_%H_%M')
  debug_bucket_key=s3://${debug_bucket}/${granule}_${timestamp}
  aws s3 cp "$granuledir" "$debug_bucket_key" --recursive --quiet
  aws s3 cp "${outputhdf}" "s3://${debug_bucket_key}/${outputname}.hdf" --quiet
fi
