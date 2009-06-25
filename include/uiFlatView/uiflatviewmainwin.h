#ifndef uiflatviewmainwin_h
#define uiflatviewmainwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewmainwin.h,v 1.11 2009-06-25 06:15:01 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiflatviewwin.h"
#include "uimainwin.h"
#include "uigroup.h"


/*!\brief (Non-modal) main window containing one or more uiFlatViewer(s). */

mClass uiFlatViewMainWin : public uiMainWin
			, public uiFlatViewWin
{
public:

    struct Setup
    {
					Setup( const char* wintitl,
					       bool delonclose=true )
					    : wintitle_(wintitl)
					    , nrviewers_(1)
					    , nrstatusfields_(1)
					    , deleteonclose_(delonclose)
					    , withhanddrag_(false)
					    , menubar_(false)		{}
	mDefSetupMemb(BufferString,wintitle)
	mDefSetupMemb(int,nrviewers)
	mDefSetupMemb(int,nrstatusfields)
	mDefSetupMemb(bool,deleteonclose)
	mDefSetupMemb(bool,menubar)
	mDefSetupMemb(bool,withhanddrag)
    };

    			uiFlatViewMainWin(uiParent*,const Setup&);

    virtual void	start()				{ show(); }
    virtual void	setWinTitle( const char* t )	{ setCaption(t); }

    void		addControl(uiFlatViewControl*);
    void		displayInfo(CallBacker*);
    void		setInitialSize(int w,int h);

    virtual uiMainWin*	dockParent()	{ return this; }
    virtual uiParent*	viewerParent()	{ return this; }

};


#endif
