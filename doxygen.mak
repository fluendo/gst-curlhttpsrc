.PHONY: doc doc-clean doc-internal doc-tar

PACKAGE_DOC_NAME = $(top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-doc
PACKAGE_DOC_CONTENTS = $(top_builddir)/doc/html/ $(top_builddir)/doc/xml/ $(top_builddir)/doc/latex/ $(top_builddir)/doc/man/

oneplay-engine.devhelp: doc
	xsltproc -o $@ common/doc/doxygen_to_devhelp.xsl doc/xml/index.xml

doc-clean:
	rm -rf $(PACKAGE_DOC_CONTENTS) $(PACKAGE_DOC_NAME).tar*

doc-internal: doc-clean
	doxygen $(top_srcdir)/doc/Doxyfile_internal.dox
	cp -r $(top_srcdir)/common/doc/css $(top_builddir)/doc/html/
	rm -rf $(PACKAGE_DOC_NAME).tar*
	mkdir -p $(PACKAGE_DOC_NAME)/doc
	cp -R $(PACKAGE_DOC_CONTENTS) $(PACKAGE_DOC_NAME)/doc
	tar cf $(PACKAGE_DOC_NAME).tar $(PACKAGE_DOC_NAME)/
	bzip2 -9 $(PACKAGE_DOC_NAME).tar
	rm -rf $(PACKAGE_DOC_NAME)/

doc: doc-clean
	doxygen $(top_srcdir)/doc/Doxyfile_api.dox
	cp -r $(top_srcdir)/common/doc/css $(top_builddir)/doc/html/
	rm -rf $(PACKAGE_DOC_NAME).tar*
	mkdir -p $(PACKAGE_DOC_NAME)/doc
	cp -R $(PACKAGE_DOC_CONTENTS) $(PACKAGE_DOC_NAME)/doc
	tar cf $(PACKAGE_DOC_NAME).tar $(PACKAGE_DOC_NAME)/
	bzip2 -9 $(PACKAGE_DOC_NAME).tar
	rm -rf $(PACKAGE_DOC_NAME)/

clean-local: doc-clean
