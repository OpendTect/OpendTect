#ifndef uiflatviewwin_h
#define uiflatviewwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewwin.h,v 1.4 2007-03-01 12:03:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"
class uiParent;
class uiFlatViewer;
class uiFlatViewControl;


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

    virtual void	setWinTitle(const char*)	= 0;
    virtual void	start()				= 0;
    virtual void	addControl(uiFlatViewControl*)	{}

protected:

    ObjectSet<uiFlatViewer>	vwrs_;

    void			createViewers(int);
    void			cleanUp();

    virtual uiParent*		uiparent()			= 0;
    virtual void		handleNewViewer(uiFlatViewer*)	{}
};


#endif
