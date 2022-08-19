#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "uidialog.h"
#include "multiid.h"

class uiGenInput;
class uiSurfaceWrite;


/*!\brief Dialog to create a horizon with a constant Z */

mExpClass(uiEarthModel) uiCreateHorizon : public uiDialog
{ mODTextTranslationClass(uiCreateHorizon)
public:
			uiCreateHorizon(uiParent*,bool is2d);
			~uiCreateHorizon();

    MultiID		getSelID() const;

    Notifier<uiCreateHorizon> ready;

protected:

    bool		acceptOK(CallBacker*) override;

    uiGenInput*		zfld_;
    uiSurfaceWrite*	outfld_;
};
