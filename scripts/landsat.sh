#!/bin/bash

# Exit on any error
#set -o errexit

save_debug_output="$SAVE_DEBUG_OUTPUT"
exit_after_fmask="$EXIT_AFTER_FMASK"
exit_after_lasrc="$EXIT_AFTER_LASRC"

# shellcheck disable=2153
granule="$GRANULE" #"LC08_L1TP_096011_20250819_20250820_02_RT"
inputdir="/tmp/l30_input"
outputdir="/tmp/l30_int_output"

#shellcheck disable=2153
workingdir="/var/scratch"
granuledir="${workingdir}/${granule}"

# Remove tmp files on exit
# shellcheck disable=2064
trap "rm -rf $workingdir; exit" INT TERM EXIT

fmaskversion="4.7"
rename_angle_bands () {
  anglebasename=$1
  newbasename=$2
  mv "${anglebasename}_VAA.hdr" "${newbasename}_VAA.hdr"
  mv "${anglebasename}_VAA.img" "${newbasename}_VAA.img"
  mv "${anglebasename}_VZA.hdr" "${newbasename}_VZA.hdr"
  mv "${anglebasename}_VZA.img" "${newbasename}_VZA.img"
  mv "${anglebasename}_SAA.hdr" "${newbasename}_SAA.hdr"
  mv "${anglebasename}_SAA.img" "${newbasename}_SAA.img"
  mv "${anglebasename}_SZA.hdr" "${newbasename}_SZA.hdr"
  mv "${anglebasename}_SZA.img" "${newbasename}_SZA.img"
}

# Create workingdir
echo $granuledir
mkdir -p "$granuledir"


echo "Start processing granules"

#echo "Copying granule from USGS S3"
echo "copying granule to working dir"
cp -r "${inputdir}/${granule}/" "$workingdir" 

fmask="${granule}_Fmask4.tif"
fmaskbin=fmask.bin

IFS='_'
read -ra granulecomponents <<< "$granule"
date=${granulecomponents[3]:0:8}
year=${date:0:4}
month=${date:4:2}
day=${date:6:2}
pathrow=${granulecomponents[2]}
outputname="${year}-${month}-${day}_${pathrow}"
bucket_key="${bucket}/${year}-${month}-${day}/${pathrow}"


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
echo "running fmask"
run_Fmask.sh >> fmask_out.txt
echo "fmask completed"

if [ "$exit_after_fmask" == "true" ]; then
  mkdir -p "${outputdir}/${outputname}/"
  cp $fmask "${outputdir}/${outputname}/${granule}_Fmask${fmaskversion}.tif"
  echo "Fmask successfully completed. Exiting now"
  exit
fi

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

if [ "$exit_after_lasrc" == "true" ]; then
  rsync -av  ${workingdir}/ "${outputdir}/${outputname}/"
  echo "LaSRC successfully completed. Saving resampled output to $outputdir. Exiting now"
  exit
fi

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
aerosol_qa="${granule}_sr_aerosol_qa.img"
addFmaskSDS "$srhdf" "$fmaskbin" "$aerosol_qa" "$mtl" "$ACCODE" "$outputhdf"


if [ "$save_debug_output" == "false" ]; then
  echo "saving output to ${outputdir}/${year}-${month}-${day}/${pathrow}"
  mkdir -p "${outputdir}/${year}-${month}-${day}/${pathrow}"
  cp "${outputhdf}" "${outputdir}/${year}-${month}-${day}/${pathrow}"
  rsync -av --include="*_VAA.hdr" --include="*_VZA.hdr" --include="*_VZA.img" \
   --include="*_SAA.hdr" --include="*_SAA.img" --include="*_SZA.hdr" --include="*_SZA.img" \
   --include="*_VAA.img" --exclude="*" "${granuledir}" \
  "${outputdir}/${year}-${month}-${day}/${pathrow}"

elif  [ "$save_debug_output" == "true" ]; then
  # Copy all intermediate files to debug bucket.
  # note we do not want the granule name in the output path. Just the date.
  # hls-landsat-tile tiles all granules for this date in this dir to MGRS
  echo "saving intermediate files to ${outputdir}/${year}-${month}-${day}/${pathrow}"
  mkdir -p "${outputdir}/${year}-${month}-${day}/${pathrow}"
  timestamp=$(date +'%Y_%m_%d_%H_%M')
  rsync -av "$granuledir" "${outputdir}/${year}-${month}-${day}/${pathrow}"

else
  echo "no files copied. check save_debug_output flag"
fi

#if [[ -z "$DEBUG_BUCKET" ]]; then
#  aws s3 cp "${outputhdf}" "s3://${bucket_key}/${outputname}.hdf"
#  aws s3 cp "$granuledir" "s3://${bucket_key}" --exclude "*" --include "*_VAA.img" \
#    --include "*_VAA.hdr" --include "*_VZA.hdr" --include "*_VZA.img" \
#    --include "*_SAA.hdr" --include "*_SAA.img" --include "*_SZA.hdr" \
#    --include "*_SZA.img" --recursive --quiet
#else
#  debug_bucket="$DEBUG_BUCKET"
#  # Copy all intermediate files to debug bucket.
#  echo "Copy files to debug bucket"
#  timestamp=$(date +'%Y_%m_%d_%H_%M')
#  debug_bucket_key=s3://${debug_bucket}/${granule}_${timestamp}
#  aws s3 cp "$granuledir" "$debug_bucket_key" --recursive --quiet
#fi
