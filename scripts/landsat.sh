#/bin/bash
export OMP_NUM_THREADS=2

# Exit on any error
set -o errexit

jobid="$AWS_BATCH_JOB_ID"
granule="$GRANULE"
bucket="$OUTPUT_BUCKET"
inputbucket="$INPUT_BUCKET"
workingdir="/var/scratch/${jobid}"
granuledir="${workingdir}/${granule}"
debug_bucket="$DEBUG_BUCKET"
replace_existing="$REPLACE_EXISTING"
ACCODE=Lasrc

# Remove tmp files on exit
trap "rm -rf $workingdir; exit" INT TERM EXIT

# Create workingdir
mkdir -p "$workingdir"

fmask="${granule}_Fmask4.tif"
fmaskbin=fmask.bin

echo "Start processing granules"

# Read into an array as tokens separated by IFS
IFS='_'
read -ra ADDR <<< "$granule"

# Format GCS url and download
url="gs://gcp-public-data-landsat/LC08/01/${ADDR[2]:0:3}/${ADDR[2]:3:5}/${granule}/"
gsutil -m cp -r "$url" "$workingdir"

exit_if_exists () {
  if [ ! -z "$replace_existing" ]; then
    # Check if output folder key exists
    exists=$(aws s3 ls "${bucket_key}/" | wc -l)
    if [ ! "$exists" = 0 ]; then
      echo "Output product already exists.  Not replacing"
      exit 4
    fi
  fi
}

# Check solar zenith angle.
echo "Check solar azimuth"
solar_zenith_valid=$(check_solar_zenith.py -i "$granuledir")
if [ "$solar_zenith_valid" == "invalid" ]; then
  echo "Invalid solar zenith angle. Exiting now"
  exit 3
fi

# Enter working directory
cd "$granuledir"

# Run Fmask
/usr/local/MATLAB/application/run_Fmask_4_1.sh /usr/local/MATLAB/v96

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

mtl="${granule}_MTL.txt"
espa_xml="${granule}.xml"
hls_espa_xml="${granule}_hls.xml"
srhdf="sr.hdf"
outputhdf="${granule}.hdf"

# Convert to espa format
convert_lpgs_to_espa --mtl="$mtl"

# Run lasrc
do_lasrc_landsat.py --xml "$espa_xml"

# Create ESPA xml file using HLS v1.5 band names
alter_sr_band_names.py -i "$espa_xml" -o "$hls_espa_xml"

# Convert ESPA xml file to HDF
convert_espa_to_hdf --xml="$hls_espa_xml" --hdf="$srhdf"

# Run addFmaskSDS
addFmaskSDS "$srhdf" "$fmaskbin" "$mtl" "$ACCODE" "$outputhdf"

if [ -z "$debug_bucket" ]; then
  aws s3 cp "${outputhdf}" "s3://${bucket}/${outputhdf}"
else
  # Copy all intermediate files to debug bucket.
  debug_bucket_key=s3://${debug_bucket}/${granule}
  aws s3 cp "$granuledir" "$debug_bucket_key" --recursive
fi
