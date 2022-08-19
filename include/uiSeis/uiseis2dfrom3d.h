#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uistring.h"

class uiSeisSel;
class uiSeis2DSubSel;

mExpClass(uiSeis) uiSeis2DFrom3D : public uiDialog
{ mODTextTranslationClass(uiSeis2DFrom3D);
public:
			uiSeis2DFrom3D(uiParent*);
			~uiSeis2DFrom3D();

protected:

    void		cubeSel(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiSeisSel*		data3dfld_;
    uiSeis2DSubSel*	subselfld_;
    uiSeisSel*		data2dfld_;
};
