#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
