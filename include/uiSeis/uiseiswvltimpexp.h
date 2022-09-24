#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"

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

    Table::FormatDesc&	fd_;

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;
    uiGenInput*		scalefld_;
    uiIOObjSel*		wvltfld_;

    void		inputChgd(CallBacker*);
    bool		acceptOK(CallBacker*) override;
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
    bool		acceptOK(CallBacker*) override;
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
    bool		acceptOK(CallBacker*) override;
};
