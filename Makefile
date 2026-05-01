# DPDK app Makefile
APP = dpdk-app
SRCS = main.c

# DPDK flags
PKGCONF = pkg-config
CFLAGS += $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs libdpdk)

# Build
$(APP): $(SRCS)
	gcc $(CFLAGS) $(SRCS) -o $(APP) $(LDFLAGS)

clean:
	rm -f $(APP)
