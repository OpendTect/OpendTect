#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

#include "pickset.h"
#include "uigeninput.h"
#include "uiioobjsel.h"


namespace EM { class PolygonZChanger; }
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
