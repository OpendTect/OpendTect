#ifndef uiflatviewwin_h
#define uiflatviewwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewwin.h,v 1.1 2007-02-20 18:15:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
class uiFlatViewer;
class uiFlatViewerControl;


/*!\brief (Non-modal) window containing one or more uiFlatViewer(s).

  will clean up the mess when it's destroyed, in particular release all
  datapacks attached to the viewers.

*/

class uiFlatViewWin : public uiMainWin
{
public:

    struct Setup
    {
			Setup( const char* wintitl )
			    : wintitle_(wintitl)
			    , nrstatusfields_(0)
			    , nrviewers_(1)
			    , menubar_(false)		{}
	mDefSetupMemb(BufferString,wintitle)
	mDefSetupMemb(int,nrstatusfields)
	mDefSetupMemb(int,nrviewers)
	mDefSetupMemb(bool,menubar)
    };

    			uiFlatViewWin(uiParent*,const Setup&);

    uiFlatViewer&	viewer( int idx=0 )	{ return *vwrs_[idx]; }
    int			nrViewers() const	{ return vwrs_.size(); }

    void		addNullOnClose( uiFlatViewWin** p )
						{ tonull_ += p; }
    			//!< On close: *p = 0;

protected:

    ObjectSet<uiFlatViewer>	vwrs_;
    ObjectSet<uiFlatViewWin*>	tonull_;

    void			addViewer();
    virtual bool		closeOK();
};


#endif
