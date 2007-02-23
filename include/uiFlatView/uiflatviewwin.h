#ifndef uiflatviewwin_h
#define uiflatviewwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewwin.h,v 1.2 2007-02-23 14:26:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"
class uiFlatViewer;
class uiParent;


/*!\brief Base class for windows containing one or more uiFlatViewer(s).

  will clean up the mess when it's destroyed, in particular release all
  datapacks attached to the viewers.

*/

class uiFlatViewWin
{
public:

    virtual		~uiFlatViewWin()	{}

    uiFlatViewer&	viewer( int idx=0 )	{ return *vwrs_[idx]; }
    int			nrViewers() const	{ return vwrs_.size(); }

    void		setDarkBG(bool);
    void		addNullOnClose( uiFlatViewWin** p )
						{ tonull_ += p; }
    			//!< On close: *p = 0;

    virtual void	setWinTitle(const char*)	= 0;
    virtual void	start()				= 0;

protected:

    ObjectSet<uiFlatViewer>	vwrs_;
    ObjectSet<uiFlatViewWin*>	tonull_;

    void			createViewers(int);
    void			cleanUp();

    virtual uiParent*		uiparent()			= 0;
    virtual void		handleNewViewer(uiFlatViewer*)	= 0;
};


#endif
