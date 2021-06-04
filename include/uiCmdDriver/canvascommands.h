#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"
#include "uigraphicsviewbase.h"

/*!\brief %Command Drive*/

namespace CmdDrive
{

mStartDeclCmdClass( uiCmdDriver, CanvasMenu, UiObjectCmd )	mEndDeclCmdClass


mExpClass(uiCmdDriver) GraphicsViewMenuActivator: public Activator
{
public:
		    GraphicsViewMenuActivator(const uiGraphicsViewBase& obj)
			: actobj_( const_cast<uiGraphicsViewBase&>(obj) )
		    {}
    void	    actCB(CallBacker*);

protected:
    uiGraphicsViewBase&   actobj_;
};


mStartDeclCmdClass( uiCmdDriver, NrCanvasMenuItems, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsCanvasMenuItemOn, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetCanvasMenuItem, UiObjQuestionCmd )
    mEndDeclCmdClass

mStartDeclComposerClass( uiCmdDriver,CanvasMenu,CmdComposer,uiGraphicsViewBase )
mEndDeclComposerClass


}; // namespace CmdDrive

