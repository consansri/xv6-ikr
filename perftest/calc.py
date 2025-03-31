import re

with open("log.txt") as ifile:
    with open("plog.txt", "w") as ofile:
        ilines = ifile.readlines()
        odatabfr = ""
        currentstrat = 0

        for iline in ilines:
            valres = re.search("waymask ([0-9]{1}) : ([0-9]*)/([0-9]*)", iline)

            stratres = re.search("test for strategy ([0-9]{1})", iline)
            if stratres != None:
                currentstrat = int(stratres.group(1))

            oline = iline.replace("\n", "")
            if valres != None:
                ways = int(valres.group(1))
                if valres.group(1) != "9":
                    if currentstrat == 1:
                        odatabfr = odatabfr + "({0},{1:.3f})".format(ways, (int(valres.group(2))/int(valres.group(3)) * 100))
                    elif currentstrat in [2,3] and ways in [1,2,4,8]:
                        odatabfr = odatabfr + "({0},{1:.3f})".format(ways, (int(valres.group(2))/int(valres.group(3)) * 100))


            if "starting performance test" in iline or iline == "\n":
                oline = odatabfr + "\n" + oline
                odatabfr = ""

            ofile.write(oline + "\n")

                
        