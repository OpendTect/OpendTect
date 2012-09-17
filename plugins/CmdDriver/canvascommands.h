#ifndef canvascommands_h
#define canvascommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id: canvascommands.h,v 1.5 2011/07/27 09:22:49 cvsjaap Exp $
 ________________________________________________________________________

-*/

#include "command.h"
#include "cmdcomposer.h"

class uiCanvas;
class uiGraphicsView;

namespace CmdDrive
{

class CmdDriver;


mStartDeclCmdClass( CanvasMenu, UiObjectCmd )		mEndDeclCmdClass

#define mDeclCanvasMenuActivator( typ, objclass ) \
\
    mClass typ##Activator: public Activator \
    { \
    public: \
		    typ##Activator(const objclass& obj) \
			: actobj_( const_cast<objclass&>(obj) ) \
		    {} \
	void	    actCB(CallBacker*); \
    protected: \
	objclass&   actobj_; \
    };

mDeclCanvasMenuActivator( CanvasMenu, uiCanvas )
mDeclCanvasMenuActivator( GraphicsViewMenu, uiGraphicsView )

mStartDeclCmdClass( NrCanvasMenuItems, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( IsCanvasMenuItemOn, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( GetCanvasMenuItem, UiObjQuestionCmd )	mEndDeclCmdClass

mStartDeclComposerClass( CanvasMenu, CmdComposer )	mEndDeclComposerClass


}; // namespace CmdDrive

#endif
