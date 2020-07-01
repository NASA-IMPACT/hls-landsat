#! /usr/bin/env python
import sys, getopt
import datetime


def main(argv):
    year = ''
    month = ''
    day = ''
    try:
        opts, args = getopt.getopt(argv,"y:m:d:h",["year=","month=","day="])
    except getopt.GetoptError:
        print ('get_doy.py -y 2019 -m 01 -d 02')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print ('get_doy.py -y 2019 -m 01 -d 02')
            sys.exit()
        elif opt in ("-y", "--year"):
            year = int(arg)
        elif opt in ("-m", "--month"):
            month = int(arg.lstrip("0"))
        elif opt in ("-d", "--day"):
            day = int(arg.lstrip("0"))
    date = datetime.date(year, month, day)
    day_of_year = date.timetuple().tm_yday
    # derive_s2nbar requires a three digit doy
    sys.stdout.write(str(day_of_year).zfill(3))

if __name__ == "__main__":
    main(sys.argv[1:])
