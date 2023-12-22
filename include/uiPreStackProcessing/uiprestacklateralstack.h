#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
class uiGenInput;

namespace PreStack
{

class LateralStack;
class Processor;

mExpClass(uiPreStackProcessing) uiLateralStack : public uiDialog
{ mODTextTranslationClass(uiLateralStack);
public:
    static void		initClass();
			uiLateralStack(uiParent*,LateralStack*);
			~uiLateralStack();

protected:
    bool		acceptOK(CallBacker*) override;
    static uiDialog*	create(uiParent*,Processor*);

    LateralStack*	processor_;
    uiGenInput*		stepoutfld_;
    uiGenInput*		iscrossfld_ = nullptr;
};

} // namespace PreStack
