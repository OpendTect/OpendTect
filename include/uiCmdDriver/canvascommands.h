#ifndef canvascommands_h
#define canvascommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id: canvascommands.h,v 1.1 2012-09-17 12:38:31 cvsjaap Exp $
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

class uiGraphicsViewBase;

namespace CmdDrive
{

class CmdDriver;


mStartDeclCmdClass( CanvasMenu, UiObjectCmd )		mEndDeclCmdClass


mClass(CmdDriver) GraphicsViewMenuActivator: public Activator
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

mStartDeclComposerClass( CanvasMenu, CmdComposer )	mEndDeclComposerClass


}; // namespace CmdDrive

#endif

