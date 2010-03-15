#ifndef uisegyexp_h
#define uisegyexp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyexp.h,v 1.8 2010-03-15 09:27:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "seistype.h"
#include "iopar.h"
#include "odusgclient.h"
class uiSeisSel;
class uiCheckBox;
class uiSeisTransfer;
class uiSEGYFilePars;
class uiSEGYFileSpec;
class uiSEGYExpTxtHeader;


mClass uiSEGYExp : public uiDialog
		 , public Usage::Client
{
public:

			uiSEGYExp(uiParent*,Seis::GeomType);
			~uiSEGYExp();

protected:

    Seis::GeomType	geom_;
    bool		autogentxthead_;
    BufferString	hdrtxt_;
    IOPar		pars_;
    int			selcomp_;

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
