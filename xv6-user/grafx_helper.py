import colorsys

rgbvals = ["ffffff", "ffff00", "ff6500", "dc0000", "ff0097", "360097", "0000ca", "0097ff", \
  "00a800", "006500", "653600", "976536", "b9b9b9", "868686", "454545", "000000"]
for rgb in rgbvals:
    print("{", end = "")
    for ix, val in enumerate(colorsys.rgb_to_hls(int(rgb[0:2], 16)/255, int(rgb[2:4], 16)/255, int(rgb[4:6], 16)/255)):
        if ix == 2:
            print(int(val*255), end = "")
        else:
            print(int(val*255), end = ",")
    print("},")