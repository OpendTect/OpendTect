#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiFlatViewMainWin();

    void		start() override	{ show(); }
    void		setWinTitle( const uiString& t ) override
			    { setCaption(t); }

    void		addControl(uiFlatViewControl*) override;
    void		displayInfo(CallBacker*);
    void		setInitialSize(int w,int h) override;

    uiMainWin*		dockParent() override	{ return this; }
    uiParent*		viewerParent() override { return this; }

};
