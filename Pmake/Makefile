#_______________________Pmake___________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
#	Author:		A.H. Bril
#	Date:		2003
#	RCS:		$Id: Makefile,v 1.21 2008/04/22 06:02:21 cvsnanne Exp $
#_______________________________________________________________________________
#
# Make file for the Pmake subdir
# Generates the module-dependency make-include

MODFILE := $(WORK)/Pmake/ModDeps.od
GENDOCFILE := $(WORK)/doc/Programmer/Generated/index.html
ifeq ($(HDIR),win)
    MODFILE := $(shell cygpath -u $(MODFILE))
    GENDOCFILE := $(shell cygpath -u $(GENDOCFILE))
endif

EXTRAMODDEPS := $(filter-out ModDeps.od,$(wildcard ModDeps.*))
ifeq ($(EXTRAMODDEPS),)
    all : make.od.ModDeps
else
    EXTRAMODDEPS := $(subst ModDeps.,,$(EXTRAMODDEPS))
    EXTRAMODDEPS := $(addprefix make.,$(EXTRAMODDEPS))
    EXTRAMODDEPS := $(addsuffix .ModDeps,$(EXTRAMODDEPS))
    all : make.od.ModDeps $(EXTRAMODDEPS)
endif

make.od.ModDeps : $(WORK)/Pmake/ModDeps.od
	@echo "Making make.od.ModDeps ..." 
	@-$(PMAKE)/bin/$(HDIR)/GenModDeps $(MODFILE) make.od.ModDeps

MODDEPSTEMPFILE := tmp_ModDeps
make.%.ModDeps : $(WORK)/Pmake/ModDeps.od $(WORK)/Pmake/ModDeps.%
	@echo "Making make.$*.ModDeps ..." 
	@cat ModDeps.od ModDeps.$* > $(MODDEPSTEMPFILE)
	@-$(PMAKE)/bin/$(HDIR)/GenModDeps $(MODDEPSTEMPFILE) make.$*.ModDeps
	@rm -f $(MODDEPSTEMPFILE)

$(GENDOCFILE) : $(MODFILE)
doc : $(GENDOCFILE)
	@echo "Creating $(GENDOCFILE) ..." 
	@-$(WORK)/bin/$(PLFSUBDIR)/GenModDeps --html ModDeps.od $(GENDOCFILE)

.q:
	@-echo "$($(v))"
