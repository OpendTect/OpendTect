#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiIOObjSel;
class uiSeisSubSel;
class uiVelSel;

namespace PreStack
{
    class uiAngleCompGrp;
    class AngleMuteComputer;

mExpClass(uiPreStackProcessing) uiAngleMuteComputer : public uiDialog
{ mODTextTranslationClass(uiAngleMuteComputer);
public:
			uiAngleMuteComputer(uiParent*);
			~uiAngleMuteComputer();
protected:


    uiAngleCompGrp*	anglecompgrp_;
    AngleMuteComputer*	processor_;
    uiSeisSubSel*	subsel_;
    uiIOObjSel*		mutedeffld_;

    bool		acceptOK(CallBacker*) override;
};

} // namespace PreStack
