#ifndef uisegyexp_h
#define uisegyexp_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyexp.h,v 1.4 2009-01-08 08:31:03 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "seistype.h"
#include "iopar.h"
class uiSeisSel;
class uiCheckBox;
class uiSeisTransfer;
class uiSEGYFilePars;
class uiSEGYFileSpec;
class uiSEGYExpTxtHeader;


mClass uiSEGYExp : public uiDialog
{
public:

			uiSEGYExp(uiParent*,Seis::GeomType);
			~uiSEGYExp();

protected:

    CtxtIOObj&		ctio_;
    Seis::GeomType	geom_;
    bool		autogentxthead_;
    BufferString	hdrtxt_;
    IOPar		pars_;

    uiSeisSel*		seissel_;
    uiSeisTransfer*	transffld_;
    uiSEGYFilePars*	fpfld_;
    uiSEGYFileSpec*	fsfld_;
    uiSEGYExpTxtHeader*	txtheadfld_;
    uiCheckBox*		morebut_;

    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    friend class	uiSEGYExpMore;
    friend class	uiSEGYExpTxtHeader;
    bool		doWork(const IOObj&,const IOObj&,
	    			const char*,const char*);

};


#endif
