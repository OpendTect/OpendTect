#ifndef uiflatviewwin_h
#define uiflatviewwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewwin.h,v 1.15 2011/06/29 11:15:32 cvsbruno Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include <iostream>
class uiParent;
class uiMainWin;
class uiFlatViewer;
class uiFlatViewControl;
class IOPar;


/*!\brief Base class for windows containing one or more uiFlatViewer(s).

  will clean up the mess when it's destroyed, in particular release all
  datapacks attached to the viewers.

*/

mClass uiFlatViewWin
{
public:

    virtual		~uiFlatViewWin()	{}

    uiFlatViewer&	viewer( int idx=0 )	{ return *vwrs_[idx]; }
    const uiFlatViewer&	viewer( int idx=0 ) const { return *vwrs_[idx]; }
    int			nrViewers() const	{ return vwrs_.size(); }

    void		setDarkBG(bool);

    virtual void	setWinTitle(const char*)	= 0;
    virtual void	start()				= 0;
    virtual void	addControl(uiFlatViewControl*)	{}
    virtual uiMainWin*	dockParent()			= 0;
    virtual uiParent*	viewerParent()			= 0;

    virtual void	setInitialSize(int w,int h);
    virtual void	fillPar(IOPar&) const		{}
    virtual void	usePar(const IOPar&) 		{}

protected:

    ObjectSet<uiFlatViewer>	vwrs_;

    void			makeInfoMsg(BufferString&,IOPar&) const;
    void			createViewers(int,bool withhanddrag = false);
    void			cleanUp();

    virtual void		handleNewViewer(uiFlatViewer*)	{}
};


#endif
