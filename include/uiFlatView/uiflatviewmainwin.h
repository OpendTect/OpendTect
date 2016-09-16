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
#include "uimainwin.h"
#include "uigroup.h"

/*!
\brief (Non-modal) main window containing one or more uiFlatViewer(s).
*/

mExpClass(uiFlatView) uiFlatViewMainWin : public uiMainWin
			, public uiFlatViewWin
{
public:

    struct Setup
    {
					Setup( const uiString& wintitl,
					       bool delonclose=true )
					    : wintitle_(wintitl)
					    , nrviewers_(1)
					    , nrstatusfields_(1)
					    , deleteonclose_(delonclose)
					    , menubar_(false)		{}
	mDefSetupMemb(uiString,wintitle)
	mDefSetupMemb(int,nrviewers)
	mDefSetupMemb(int,nrstatusfields)
	mDefSetupMemb(bool,deleteonclose)
	mDefSetupMemb(bool,menubar)
    };

    			uiFlatViewMainWin(uiParent*,const Setup&);

    virtual void	start()				{ show(); }
    virtual void	setWinTitle( const uiString& t ){ setCaption(t); }

    void		addControl(uiFlatViewControl*);
    void		displayInfo(CallBacker*);
    void		setInitialSize(int w,int h);

    virtual uiMainWin*	dockParent()	{ return this; }
    virtual uiParent*	viewerParent()	{ return this; }

};
