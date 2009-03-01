all: supplib tfmvlib tst

supplib:
	(cd supp_src; make)
tfmvlib:
	(cd tfmv_src; make)
tst:
	(cd test; make)

clean:
	(cd supp_src; make clean)
	(cd test; make clean)
