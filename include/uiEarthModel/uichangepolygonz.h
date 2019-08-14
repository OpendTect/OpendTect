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

mExpClass(uiEarthModel) uiChangePolygonZ : public uiDialog
{
    mODTextTranslationClass(uiChangePolygonZ);
public:
    uiChangePolygonZ(uiParent*,Pick::Set&);
    ~uiChangePolygonZ();

protected:
    bool		acceptOK();
    void		changeZvalCB(CallBacker*);

    uiGenInput*		changezfld_;
    uiGenInput*		constzfld_;
    uiIOObjSel*		horinpfld_;
    Pick::Set&		set_;
};
