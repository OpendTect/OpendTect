#ifndef uisegyexp_h
#define uisegyexp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "iopar.h"
#include "seistype.h"
#include "uistring.h"

class IOObj;
class uiCheckBox;
class uiSEGYFilePars;
class uiSEGYFileSpec;
class uiSEGYExpTxtHeader;
class uiSeisSel;
class uiSeisTransfer;


/*\brief SEG-Y exporting dialog */


mExpClass(uiSeis) uiSEGYExp : public uiDialog
{ mODTextTranslationClass(uiSEGYExp);
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
    uiCheckBox*		morebox_;
    uiCheckBox*		manipbox_;

    void		inpSel(CallBacker*);
    void		batchChg(CallBacker*);
    bool		acceptOK(CallBacker*);

    friend class	uiSEGYExpMore;
    friend class	uiSEGYExpTxtHeader;
    bool		doWork(const IOObj&,const IOObj&,const char*);

};


#endif

