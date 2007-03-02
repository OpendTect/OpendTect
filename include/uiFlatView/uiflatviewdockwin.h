#ifndef uiflatviewdockwin_h
#define uiflatviewdockwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewdockwin.h,v 1.4 2007-03-02 14:28:02 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiflatviewwin.h"
#include "uidockwin.h"


/*!\brief (Non-modal) main window containing one or more uiFlatViewer(s). */

class uiFlatViewDockWin : public uiDockWin
			, public uiFlatViewWin
{
public:

    struct Setup
    {
					Setup( const char* nm )
					    : name_(nm)
					    , nrviewers_(1)		{}
	mDefSetupMemb(BufferString,	name)
	mDefSetupMemb(int,		nrviewers)
    };


    			uiFlatViewDockWin(uiParent*,const Setup&);
    			~uiFlatViewDockWin();

    virtual void	start()				{ display(true); }
    virtual void	setWinTitle( const char* t )	{ setDockName(t); }
    virtual uiMainWin*	dockParent()			{ return mainwin(); }
    virtual uiParent*	viewerParent()			{ return this; }

};


#endif
