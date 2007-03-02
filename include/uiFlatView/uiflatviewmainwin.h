#ifndef uiflatviewmainwin_h
#define uiflatviewmainwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewmainwin.h,v 1.3 2007-03-02 14:28:02 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiflatviewwin.h"
#include "uimainwin.h"


/*!\brief (Non-modal) main window containing one or more uiFlatViewer(s). */

class uiFlatViewMainWin : public uiMainWin
			, public uiFlatViewWin
{
public:

    struct Setup
    {
					Setup( const char* wintitl )
					    : wintitle_(wintitl)
					    , nrstatusfields_(1)
					    , nrviewers_(1)
					    , menubar_(false)		{}
	mDefSetupMemb(BufferString,	wintitle)
	mDefSetupMemb(int,		nrstatusfields)
	mDefSetupMemb(int,		nrviewers)
	mDefSetupMemb(bool,		menubar)
    };


    			uiFlatViewMainWin(uiParent*,const Setup&);

    virtual void	start()				{ show(); }
    virtual void	setWinTitle( const char* t )	{ setCaption(t); }

    void		addControl(uiFlatViewControl*);

    virtual uiMainWin*	dockParent()			{ return this; }
    virtual uiParent*	viewerParent()			{ return this; }

protected:

    virtual bool	closeOK()
    			{ cleanUp(); return true; }

};


#endif
