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

    void	start() override			{ display(true); }
    void	setWinTitle(const uiString& t) override { setDockName(t); }
    uiMainWin*	dockParent() override			{ return mainwin(); }
    uiParent*	viewerParent() override			{ return this; }

};


