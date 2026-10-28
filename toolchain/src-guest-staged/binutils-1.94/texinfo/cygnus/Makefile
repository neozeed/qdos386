# $Id: Makefile,v 1.1 1991/06/07 00:48:56 pesch Exp $
# Makefile for Cygnus texinfo configuration

# Set TXIDIR to a directory in TeX's TEXINPUTS in your system
TXIDIR = /limbo/tex/lib/inputs

all: $(TXIDIR)/texinfo.tex $(TXIDIR)/fsf-texi.tex $(TXIDIR)/texiplus.tex

$(TXIDIR)/texinfo.tex: texinfo.tex
	rm -f $(TXIDIR)/texinfo.tex
	(CWD=`pwd` ; \
	cd $(TXIDIR); ln -s $$CWD/texinfo.tex . || \
		ln $$CWD/texinfo.tex . || \
		cp $$CWD/texinfo.tex .)

$(TXIDIR)/texiplus.tex: texiplus.tex
	rm -f $(TXIDIR)/texiplus.tex
	(CWD=`pwd` ; \
	cd $(TXIDIR); ln -s $$CWD/texiplus.tex . || \
		ln $$CWD/texiplus.tex . || \
		cp $$CWD/texiplus.tex .)

$(TXIDIR)/fsf-texi.tex: ../fsf/texinfo.tex
	rm -f $(TXIDIR)/fsf-texi.tex
	(CWD=`pwd` ; \
	cd $(TXIDIR); ln -s $$CWD/../fsf/texinfo.tex fsf-texi.tex || \
		ln $$CWD/../fsf/texinfo.tex fsf-texi.tex || \
		cp $$CWD/../fsf/texinfo.tex fsf-texi.tex )

