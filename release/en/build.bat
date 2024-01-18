rem make all
cd..\..
nmake /f xlcd.mak all
nmake /f xlchk.mak all
nmake /f xlcopy.mak all
nmake /f xldel.mak all
nmake /f xldir.mak all
nmake /f xlmd.mak all
nmake /f xlrd.mak all
nmake /f xlren.mak all
cd release\en
rem copy everything
copy ..\..\*.exe .
del lfn.zip
c:\util\pkzip\pkzip -a -ex lfn.zip @packlist.txt
copy ..\..\*.cpp .
copy ..\..\*.inc .
copy ..\..\*.h .
del lfnsrc.zip
c:\util\pkzip\pkzip -a -ex lfnsrc.zip *.cpp *.inc *.h source.txt
