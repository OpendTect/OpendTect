#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "iopar.h"
#include "uistring.h"

class IOObj;
class uiBatchJobDispatcherSel;
class uiCheckBox;
class uiGenInput;
class uiSEGYExpTxtHeader;
class uiSEGYFilePars;
class uiSEGYFileSpec;
class uiSeisSel;
class uiSeisTransfer;
namespace Coords { class uiCoordSystemSel; }


/*\brief SEG-Y exporting dialog */

mExpClass(uiSEGYTools) uiSEGYExp : public uiDialog
{ mODTextTranslationClass(uiSEGYExp);
public:

			uiSEGYExp(uiParent*,Seis::GeomType);
			~uiSEGYExp();

protected:

    const Seis::GeomType geom_;
    bool		autogentxthead_;
    BufferString	hdrtxt_;
    IOPar		pars_;

    uiSeisSel*		seissel_;
    uiSeisTransfer*	transffld_;
    uiSEGYFilePars*	fpfld_;
    uiSEGYFileSpec*	fsfld_;
    uiSEGYExpTxtHeader*	txtheadfld_;
    uiGenInput*		txtheadfmtsel_;
    uiCheckBox*		morebox_;
    uiCheckBox*		manipbox_;
    uiBatchJobDispatcherSel* batchfld_;

    uiGenInput*		othercrsfld_;
    Coords::uiCoordSystemSel* coordsysselfld_;

    void		inpSel(CallBacker*);
    void		subSelCB(CallBacker*);
    void		crsCB(CallBacker*);
    void		batchChg(CallBacker*);
    void		showSubselCB(CallBacker*);
    void		updateTextHdrCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		fillOutputName();

    friend class	uiSEGYExpMore;
    friend class	uiSEGYExpTxtHeader;

    bool		doWork(const IOObj&,const IOObj&);
    void		getTextHeader(BufferString&);
    void		generateAutoTextHeader(BufferString&) const;

};
