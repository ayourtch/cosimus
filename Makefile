all: supplib fmvlib tst

supplib:
	(cd supp_src; make)
fmvlib:
	(cd fmv_src; make)
tst:
	(cd test; make)

clean:
	(cd supp_src; make clean)
	(cd fmv_src; make clean)
	(cd test; make clean)
