#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006 / Dec 2009
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }


mExpClass(uiSeis) uiSeisWvltImp : public uiDialog
{ mODTextTranslationClass(uiSeisWvltImp)
public:
			uiSeisWvltImp(uiParent*);
			~uiSeisWvltImp();

    MultiID		selKey() const;

protected:

    CtxtIOObj&		ctio_;
    Table::FormatDesc&	fd_;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		scalefld_;
    uiIOObjSel*		wvltfld_;

    void		inputChgd(CallBacker*);
    bool		acceptOK(CallBacker*);
};


mExpClass(uiSeis) uiSeisWvltExp : public uiDialog
{ mODTextTranslationClass(uiSeisWvltExp)
public:
			uiSeisWvltExp(uiParent*);
			~uiSeisWvltExp();

protected:

    uiIOObjSel*		wvltfld_;
    uiFileInput*	outpfld_;
    uiGenInput*		addzfld_;

    void		inputChgd(CallBacker*);
    bool		acceptOK(CallBacker*);
};


mExpClass(uiSeis) uiSeisWvltCopy : public uiDialog
{ mODTextTranslationClass(uiSeisWvltCopy)
public:
			uiSeisWvltCopy(uiParent*,const IOObj*);
			~uiSeisWvltCopy();

    MultiID		getMultiID() const;

protected:

    uiIOObjSel*		wvltinfld_;
    uiIOObjSel*		wvltoutfld_;
    uiGenInput*		scalefld_;

    void		inputChgd(CallBacker*);
    bool		acceptOK(CallBacker*);
};

