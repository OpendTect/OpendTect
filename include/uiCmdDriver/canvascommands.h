#ifndef canvascommands_h
#define canvascommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"
#include "uigraphicsviewbase.h"


namespace CmdDrive
{

mStartDeclCmdClass( CanvasMenu, UiObjectCmd )		mEndDeclCmdClass


mClass(uiCmdDriver) GraphicsViewMenuActivator: public Activator
{
public:
		    GraphicsViewMenuActivator(const uiGraphicsViewBase& obj)
			: actobj_( const_cast<uiGraphicsViewBase&>(obj) )
		    {}
    void	    actCB(CallBacker*);

protected:
    uiGraphicsViewBase&   actobj_;
};


mStartDeclCmdClass( NrCanvasMenuItems, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( IsCanvasMenuItemOn, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( GetCanvasMenuItem, UiObjQuestionCmd )	mEndDeclCmdClass

mStartDeclComposerClass( CanvasMenu, CmdComposer, uiGraphicsViewBase )
mEndDeclComposerClass


}; // namespace CmdDrive

#endif

