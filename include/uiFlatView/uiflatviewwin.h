#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/

#include "uiflatviewmod.h"

#include "sets.h"
#include "uistring.h"

class uiParent;
class uiMainWin;
class uiFlatViewer;
class uiFlatViewControl;

/*!
\brief Base class for windows containing one or more uiFlatViewer(s).

  Will clean up the mess when it's destroyed, in particular releases all
  datapacks attached to the viewers.
*/

mExpClass(uiFlatView) uiFlatViewWin
{ mODTextTranslationClass(uiFlatViewWin);
public:

    virtual		~uiFlatViewWin()	{}

    uiFlatViewer&	viewer( int idx=0 )	{ return *vwrs_[idx]; }
    const uiFlatViewer&	viewer( int idx=0 ) const { return *vwrs_[idx]; }
    int			nrViewers() const	{ return vwrs_.size(); }

    void		setDarkBG(bool);

    virtual void	setWinTitle(const uiString&)	= 0;
    virtual void	start()				= 0;
    virtual void	addControl(uiFlatViewControl*)	{}
    virtual uiMainWin*	dockParent()			= 0;
    virtual uiParent*	viewerParent()			= 0;

    virtual void	setInitialSize(int w,int h);
    virtual void	fillPar(IOPar&) const		{}
    virtual void	usePar(const IOPar&)		{}
    static void		makeInfoMsg(uiString&,IOPar&);

protected:

    ObjectSet<uiFlatViewer>	vwrs_;

    void			createViewers(int);
    void			cleanUp();

    virtual void		handleNewViewer(uiFlatViewer*)	{}
};
