#ifndef uiflatviewwin_h
#define uiflatviewwin_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewwin.h,v 1.7 2008-11-14 04:43:52 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include <iostream>
class uiParent;
class uiMainWin;
class uiFlatViewer;
class uiFlatViewControl;


/*!\brief Base class for windows containing one or more uiFlatViewer(s).

  will clean up the mess when it's destroyed, in particular release all
  datapacks attached to the viewers.

*/

class uiFlatViewWin
{
public:

    virtual		~uiFlatViewWin()	{ std::cout<<"uiFlatViewWin deleted ...................................."<<std::endl;}

    uiFlatViewer&	viewer( int idx=0 )	{ return *vwrs_[idx]; }
    int			nrViewers() const	{ return vwrs_.size(); }

    void		setDarkBG(bool);

    virtual void	setWinTitle(const char*)	= 0;
    virtual void	start()				= 0;
    virtual void	addControl(uiFlatViewControl*)	{}
    virtual uiMainWin*	dockParent()			= 0;
    virtual uiParent*	viewerParent()			= 0;

protected:

    ObjectSet<uiFlatViewer>	vwrs_;

    void			createViewers(int);
    void			cleanUp();

    virtual void		handleNewViewer(uiFlatViewer*)	{}
};


#endif
