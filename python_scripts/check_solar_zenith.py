#! /usr/bin/env python
import sys, getopt
from mtlutils import parsemeta

def main(argv):
    input_file = ""
    valid = "valid" 
    try:
        opts, args = getopt.getopt(argv,"i:h",["input_file="])
    except getopt.GetoptError:
        print ("check_solar_zenith.py -i _MTL.txt")
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-h":
            print ('check_solar_zenith.py -i _MTL.txt')
            sys.exit()
        elif opt in ("-i", "--input_file"):
            input_file = arg

    metadata = parsemeta(input_file)
    try:
        sun_azimuth = float(
            metadata["L1_METADATA_FILE"]["IMAGE_ATTRIBUTES"]["SUN_AZIMUTH"]
        )
    except KeyError:
        sun_azimuth = float(
            metadata["LANDSAT_METADATA_FILE"]["IMAGE_ATTRIBUTES"]["SUN_AZIMUTH"]
        )

    solar_zenith = 90 - sun_azimuth
    if (solar_zenith > 76):
        valid = "invalid"
    sys.stdout.write(valid)

if __name__ == "__main__":
    main(sys.argv[1:])
