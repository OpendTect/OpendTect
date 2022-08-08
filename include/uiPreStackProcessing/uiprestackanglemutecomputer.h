#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class CtxtIOObj;
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

    CtxtIOObj&		outctio_;

    uiAngleCompGrp*		anglecompgrp_;
    AngleMuteComputer*	processor_;
    uiSeisSubSel*	subsel_;
    uiIOObjSel*		mutedeffld_;

    bool		acceptOK(CallBacker*) override;
};


}; //namespace

