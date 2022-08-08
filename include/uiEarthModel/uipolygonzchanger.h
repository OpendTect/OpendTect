#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rahul Gogia
 Date:		July 2019
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
