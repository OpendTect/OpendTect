#ifndef uiflatviewdockwin_h
#define uiflatviewdockwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewdockwin.h,v 1.1 2007-02-23 14:26:14 cvsbert Exp $
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
					    , nrviewers_(1)
					    , closemode_(Always)	{}
	mDefSetupMemb(BufferString,	name)
	mDefSetupMemb(int,		nrviewers)
	mDefSetupMemb(CloseMode,	closemode)
    };


    			uiFlatViewDockWin(uiParent*,const Setup&);
			~uiFlatViewDockWin()		{ cleanUp(); }

    virtual void	start()				{ display(true); }
    virtual void	setWinTitle( const char* t )	{ setDockName(t); }

protected:

    virtual uiParent*	uiparent()			{ return this; }
    virtual void	handleNewViewer(uiFlatViewer*);

};


#endif
