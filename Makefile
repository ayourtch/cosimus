all: supplib tst

supplib:
	(cd supp_src; make)
tst:
	(cd test; make)

clean:
	(cd supp_src; make clean)
	(cd test; make clean)
