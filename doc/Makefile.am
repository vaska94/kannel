JADE=@JADE@
JADETEX=@JADETEX@
PDFJADETEX=@PDFJADETEX@
DVIPS=@DVIPS@
FIG2DEV=@FIG2DEV@
CONVERT=@CONVERT@

HTML_DSL=@HTML_DSL@
TEX_DSL=@TEX_DSL@
XML_DCL=@XML_DCL@


docsrcs = \
	alligata/alligata.xml \
	arch/arch.xml \
	userguide/userguide.xml \
	wtls/wtls.xml

docs = $(docsrcs:.xml=.html) $(docsrcs:.xml=.rtf) $(docsrcs:.xml=.ps) $(docsrcs:.xml=.pdf)
DOCSTARGET=@DOCSTARGET@
DOCDRAFTS=@DOCDRAFTS@

pssrcs = \
	alligata/11-1.png \
	alligata/11-3.png \
	alligata/11-4.png \
	alligata/11-5.png \
	alligata/12-10.png \
	alligata/12-11.png \
	alligata/12-12.png \
	alligata/12-13.png \
	alligata/12-2.png \
	alligata/12-3.png \
	alligata/12-4.png \
	alligata/12-5.png \
	alligata/12-6.png \
	alligata/12-7.png \
	alligata/12-8.png \
	alligata/12-9.png \
	alligata/13-5.png \
	alligata/done.png \
	alligata/licences.png \
	alligata/maininst.png \
	alligata/nearlydone.png \
	alligata/nopro.png \
	alligata/progress.png \
	alligata/wapdiagsmall.png \
	wtls/fig10o.png \
	wtls/fig1o.png \
	wtls/fig2o.png \
	wtls/fig3o.png \
	wtls/fig4o.png \
	wtls/fig4out.png \
	wtls/fig5o.png \
	wtls/fig5out.png \
	wtls/fig6o.png \
	wtls/fig6out.png \
	wtls/fig6out1.png \
	wtls/fig7o.png \
	wtls/fig8o.png \
	wtls/fig9o.png

ps = $(pssrcs:.png=.ps)  

figsrcs = \
	arch/bearerbox-arch.fig \
	arch/conf-push-session.fig \
	arch/external-interfaces.fig \
	arch/kannel-boxes.fig \
	arch/kannel-push-boxes.fig \
	arch/pushbox-threads.fig \
	arch/wap-arch.fig \
	arch/wap-session.fig \
	arch/wapbox-threads.fig \
	userguide/sms-gateway.fig \
	userguide/wap-gateway.fig

figs = $(figsrcs:.fig=.png)  $(figsrcs:.fig=.ps)

SUFFIXES = .xml .html .rtf .ps .fig .png .pdf

.xml.html:
	sed "s/#FIGTYPE#/.png/;s/#VERSION#/${GW_VERSION}/;s/#DATE#/`date +%Y.%m.%d`/;s/#DRAFTS#/${DOCDRAFTS}/" $< > $*.tmp
	${JADE} -V nochunks -t sgml -d $(HTML_DSL) $(XML_DCL) $*.tmp > $@
	rm -f $*.tmp

.xml.rtf:
	sed "s/#FIGTYPE#/.ps/;s/#VERSION#/${GW_VERSION}/;s/#DATE#/`date +%Y.%m.%d`/;s/#DRAFTS#/${DOCDRAFTS}/" $< > $*.tmp
	cd `dirname $<` && $(JADE) -o `basename $*`.rtf -t rtf -d $(TEX_DSL) $(XML_DCL) `basename $*`.tmp
	rm -f $*.tmp

.xml.ps:
	sed "s/#FIGTYPE#/.ps/;s/#VERSION#/${GW_VERSION}/;s/#DATE#/`date +%Y.%m.%d`/;s/#DRAFTS#/${DOCDRAFTS}/" $< > $*.tmp
	$(JADE) -o $*.tex -t tex -d $(TEX_DSL) $(XML_DCL) $*.tmp
	rm -f $*.tmp
	cd `dirname $<` && $(JADETEX) `basename $*`.tex >/dev/null || true
	cd `dirname $<` && $(JADETEX) `basename $*`.tex >/dev/null || true
	cd `dirname $<` && $(JADETEX) `basename $*`.tex >/dev/null || \
	    ( echo Check `dirname $<`/`basename $*`.log for errors && false)
	rm -f $*.log
	cd `dirname $<` && $(DVIPS) -q -o `basename $*`.ps `basename $*`.dvi
	rm -f $*.dvi $*.tex $*.aux 

.xml.pdf:
	sed "s/#FIGTYPE#/.png/;s/#VERSION#/${VERSION}/;s/#DATE#/`date +%Y.%m.%d`/;s/#DRAFTS#/${DOCDRAFTS}/" $< > $*.tmp
	$(JADE) -o $*.tex -t tex -d $(TEX_DSL) $(XML_DCL) $*.tmp
	rm -f $*.tmp
	cd `dirname $<` && $(JADETEX) `basename $*`.tex >/dev/null || true
	cd `dirname $<` && $(JADETEX) `basename $*`.tex >/dev/null || true
	cd `dirname $<` && $(JADETEX) `basename $*`.tex >/dev/null || true
	rm -f $*.log $*.dvi
	cd `dirname $<` && $(PDFJADETEX) `basename $*`.tex > /dev/null || true 
	test -r $*.pdf || false
	rm -f $*.log $*.tex $*.aux $*.out

.fig.png:
	$(FIG2DEV) -Lpng $< $@

.fig.ps:
	$(FIG2DEV) -Lps $< $@

.png.ps:
	$(CONVERT) $< $@

all: $(DOCSTARGET)

docs: figs ps $(docs)
no-docs:
figs: $(figs)
ps: $(ps)
pp: $(pres)

install: install-docs
install-docs: $(SUBDIRS)
	$(INSTALL) -d $(DESTDIR)$(docdir)/examples
	$(INSTALL_DATA) examples/*.conf $(DESTDIR)$(docdir)/examples
	for docfile in userguide alligata wtls ; do \
		$(INSTALL) -d $(DESTDIR)$(docdir)/$$docfile ; \
		$(INSTALL_DATA) $$docfile/$$docfile.ps $(DESTDIR)$(docdir)/$$docfile ; \
		$(INSTALL_DATA) $$docfile/$$docfile.html $(DESTDIR)$(docdir)/$$docfile ; \
		$(INSTALL_DATA) $$docfile/$$docfile.rtf $(DESTDIR)$(docdir)/$$docfile ; \
		$(INSTALL_DATA) $$docfile/$$docfile.pdf $(DESTDIR)$(docdir)/$$docfile ; \
		$(INSTALL_DATA) $$docfile/*.png $(DESTDIR)$(docdir)/$$docfile ; \
	done

clean: $(SUBDIRS)
	rm -f $(figs) $(ps) $(docs)
