#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uiflatviewwin.h"
#include "uidockwin.h"

/*!
\brief (Non-modal) main window containing one or more uiFlatViewer(s).
*/

mExpClass(uiFlatView) uiFlatViewDockWin : public uiDockWin
			, public uiFlatViewWin
{
public:

    struct Setup
    {
		    Setup( const uiString& nm )
					    : name_(nm)
					    , nrviewers_(1)		{}
	mDefSetupMemb(uiString,		name)
	mDefSetupMemb(int,		nrviewers)
    };


    			uiFlatViewDockWin(uiParent*,const Setup&);
    			~uiFlatViewDockWin();

    virtual void	start()				{ display(true); }
    virtual void	setWinTitle(const uiString& t)	{ setDockName(t); }
    virtual uiMainWin*	dockParent()			{ return mainwin(); }
    virtual uiParent*	viewerParent()			{ return this; }

};
