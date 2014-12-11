# RCSIDENT("$SiLK: lib-deps.mk 696fd479073b 2014-01-28 19:15:22Z mthomas $")

# Rules to build libraries that tools depend on

../libflowsource/libflowsource.la:
	@echo Making required library libflowsource
	cd ../libflowsource && $(MAKE) libflowsource.la

../libsilk/libsilk-thrd.la:
	@echo Making required library libsilk-thrd
	cd ../libsilk && $(MAKE) libsilk-thrd.la

../libsilk/libsilk.la:
	@echo Making required library libsilk
	cd ../libsilk && $(MAKE) libsilk.la
