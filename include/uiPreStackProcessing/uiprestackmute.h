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
class uiIOObjSel;

namespace PreStack
{

class Mute;
class Processor;

mExpClass(uiPreStackProcessing) uiMute : public uiDialog
{ mODTextTranslationClass(uiMute)
public:

    static void		initClass();
			uiMute(uiParent*,Mute*);
			~uiMute();

protected:

    Mute*		processor_;

    bool		acceptOK(CallBacker*) override;
    static uiDialog*	create(uiParent*,Processor*);

    uiIOObjSel*		mutedeffld_;
    uiGenInput*		topfld_;
    uiGenInput*		taperlenfld_;

};

} // namespace PreStack
