all: lua_lib supplib fmvlib smvlib tst cos_prog

supplib:
	(cd supp_src; make)
fmvlib:
	(cd fmv_src; make)
smvlib:
	(cd pktsmv_src; make)
tst:
	(cd test; make)
lua_lib:
	(cd lua; make)
cos_prog:
	(cd cosimus; make)

clean:
	(cd supp_src; make clean)
	(cd fmv_src; make clean)
	(cd pktsmv_src; make clean)
	(cd lua; make clean)
	(cd test; make clean)
	(cd cosimus; make clean)
