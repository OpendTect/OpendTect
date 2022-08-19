#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "pickset.h"
#include "uidialog.h"
#include "uiearthmodelmod.h"
#include "uigeninput.h"
#include "uiioobjsel.h"

namespace Pick { class Set; }

mExpClass(uiEarthModel) uiPolygonZChanger : public uiDialog
{ mODTextTranslationClass(uiPolygonZChanger)
public:
    uiPolygonZChanger(uiParent*,Pick::Set&);
    ~uiPolygonZChanger();

protected:
    bool		acceptOK(CallBacker*) override;
    void		changeZvalCB(CallBacker*);
    bool		applyZChanges(EM::PolygonZChanger&);

    uiGenInput*		isconstzfld_;
    uiGenInput*		zvalfld_;
    uiIOObjSel*		horinpfld_;
    Pick::Set&		set_;
};
