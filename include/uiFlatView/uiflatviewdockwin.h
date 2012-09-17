#ifndef uiflatviewdockwin_h
#define uiflatviewdockwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewdockwin.h,v 1.6 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewwin.h"
#include "uidockwin.h"


/*!\brief (Non-modal) main window containing one or more uiFlatViewer(s). */

mClass uiFlatViewDockWin : public uiDockWin
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
