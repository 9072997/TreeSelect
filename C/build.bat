rc /fo resource.res resource.rc
cl /EHsc /O1 /MT /Gw /Gy /GL main.cpp resource.res /link /OPT:REF /OPT:ICF /LTCG
upx.exe --ultra-brute main.exe
