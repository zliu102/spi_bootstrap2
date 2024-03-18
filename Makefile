# contrib/reservoir_sampling/Makefile

MODULE_big = spi_bootstrap2
OBJS = spi_bootstrap2.o $(WIN32RES)
EXTENSION = spi_bootstrap2
DATA = spi_bootstrap2--1.0.sql
PGFILEDESC = "spi_bootstrap2 - binary search for integer arrays"
PG_CFLAGS += -ggdb -O0

REGRESS = spi_bootstrap2

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/spi_bootstrap2
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
