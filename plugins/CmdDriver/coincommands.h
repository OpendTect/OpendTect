#ifndef coincommands_h
#define coincommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          September 2012
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "cmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"
#include "uithumbwheel.h"


namespace CmdDrive
{


mStartDeclCmdClass( Wheel, UiObjectCmd )		mEndDeclCmdClass

mClass(CmdDriver) WheelActivator: public Activator
{
public:
			WheelActivator(const uiThumbWheel&,float angle);
    void		actCB(CallBacker*);
protected:
    uiThumbWheel&	actwheel_;
    float		actangle_;
};

mStartDeclCmdClass( GetWheel, UiObjQuestionCmd )	mEndDeclCmdClass


mStartDeclComposerClassWithInit( Wheel, CmdComposer, uiThumbWheel )
protected:
    float oldvalue_;
mEndDeclComposerClass 


}; // namespace CmdDrive

#endif

