#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiGenInput;
class uiHorizon3DSel;
class uiSurfaceWrite;

mExpClass(uiEarthModel) uiHorizonMergeDlg : public uiDialog
{ mODTextTranslationClass(uiHorizonMergeDlg);
public:
			uiHorizonMergeDlg(uiParent*,bool is2d);
			~uiHorizonMergeDlg();

    MultiID		getNewHorMid() const;
    void		setInputHors(const TypeSet<MultiID>& mids);

protected:

    bool		acceptOK(CallBacker*) override;

    uiHorizon3DSel*	horselfld_;
    uiGenInput*		duplicatefld_;
    uiSurfaceWrite*	outfld_;
};
