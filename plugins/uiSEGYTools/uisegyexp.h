#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "iopar.h"
#include "uistring.h"
#include "uicoordsystem.h"

class IOObj;
class uiBatchJobDispatcherSel;
class uiCheckBox;
class uiSEGYFilePars;
class uiSEGYFileSpec;
class uiSEGYExpTxtHeader;
class uiSeisSel;
class uiSeisTransfer;


/*\brief SEG-Y exporting dialog */


mExpClass(uiSEGYTools) uiSEGYExp : public uiDialog
{ mODTextTranslationClass(uiSEGYExp);
public:

			uiSEGYExp(uiParent*,Seis::GeomType);

protected:

    Seis::GeomType	geom_;
    bool		autogentxthead_;
    BufferString	hdrtxt_;
    IOPar		pars_;

    uiSeisSel*		seissel_;
    uiSeisTransfer*	transffld_;
    uiSEGYFilePars*	fpfld_;
    uiSEGYFileSpec*	fsfld_;
    uiSEGYExpTxtHeader*	txtheadfld_;
    uiCheckBox*		morebox_;
    uiCheckBox*		manipbox_;
    uiBatchJobDispatcherSel* batchfld_;
    Coords::uiCoordSystemSel* coordsysselfld_;


    void		inpSel(CallBacker*);
    void		batchChg(CallBacker*);
    bool		acceptOK();

    friend class	uiSEGYExpMore;
    friend class	uiSEGYExpTxtHeader;
    bool		doWork(const IOObj&,const IOObj&,const char*);

public:
    static uiString	sTxtHeadTxt() 
			{ return tr("Define the SEG-Y text header. Note that:"
			    "\n- The Cnn line start and Rev.1 indicators will "
			    "always be retained \n- You can only define " 
			    "40 lines of 80 characters"); }

};
