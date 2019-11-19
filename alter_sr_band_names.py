import urllib2

from espa import XMLError, XMLInterface
from espa import MetadataError, Metadata
xml_filename = '/usr/local/LC08_L1TP_027039_20190901_20190901_01_RT.xml'
mm = Metadata(xml_filename=xml_filename)
mm.parse()
# bandName = mm.xml_object.bands.band[0].get('name')
for band in mm.xml_object.bands.iterchildren():
    if band.get('product') == 'L1TP':
        print(band.get('name'))
        mm.xml_object.bands.remove(band)

# band = mm.xml_object.bands.band[0]
# band.set('name', 'WAAT')

mm.write(xml_filename="test_out.xml")
