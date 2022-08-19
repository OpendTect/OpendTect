#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "cmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"
#include "uithumbwheel.h"


namespace CmdDrive
{


mStartDeclCmdClass( CmdDriver, Wheel, UiObjectCmd )		mEndDeclCmdClass

mExpClass(CmdDriver) WheelActivator: public Activator
{
public:
			WheelActivator(const uiThumbWheel&,float angle);
    void		actCB(CallBacker*);
protected:
    uiThumbWheel&	actwheel_;
    float		actangle_;
};

mStartDeclCmdClass( CmdDriver, GetWheel, UiObjQuestionCmd )	mEndDeclCmdClass


mStartDeclComposerClassWithInit( CmdDriver, Wheel, CmdComposer, uiThumbWheel )
protected:
    float oldvalue_;
mEndDeclComposerClass 


} // namespace CmdDrive
