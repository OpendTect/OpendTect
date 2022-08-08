#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2014
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

