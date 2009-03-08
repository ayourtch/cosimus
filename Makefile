all: lua_lib supplib fmvlib tst

supplib:
	(cd supp_src; make)
fmvlib:
	(cd fmv_src; make)
tst:
	(cd test; make)
lua_lib:
	(cd lua; make)

clean:
	(cd supp_src; make clean)
	(cd fmv_src; make clean)
	(cd lua; make clean)
	(cd test; make clean)
