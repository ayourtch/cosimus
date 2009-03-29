include ../Makefile.inc

all: clean cosimus cosimus_assets_inventory.zip


cosimus: cosimus.o
	$(CC) $(CFLAGS) -o $@ $? $(LIBS) $(LUA_LIBS) $(APP_LIBS)

cosimus_assets_inventory.zip:
	(cd cosimus_assets_inventory; zip -r ../cosimus_assets_inventory.zip .)
clean:
	$(RM) *.o cosimus cosimus_assets_inventory.zip
