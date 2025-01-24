#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"

#include "sets.h"
#include "uistring.h"

class uiParent;
class uiMainWin;
class uiFlatViewer;
class uiFlatViewControl;
namespace FlatView { class Viewer; }

/*!
\brief Base class for windows containing one or more uiFlatViewer(s).

  Will clean up the mess when it's destroyed, in particular releases all
  datapacks attached to the viewers.
*/

mExpClass(uiFlatView) uiFlatViewWin
{ mODTextTranslationClass(uiFlatViewWin);
public:
			uiFlatViewWin();
    virtual		~uiFlatViewWin();

    uiFlatViewer&	viewer( int idx=0 )	{ return *vwrs_[idx]; }
    const uiFlatViewer&	viewer( int idx=0 ) const { return *vwrs_[idx]; }
    bool		validIdx(int) const;
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
    static void		makeInfoMsg(const FlatView::Viewer&,const IOPar&,
				    uiString&);
protected:

    ObjectSet<uiFlatViewer>	vwrs_;

    void			createViewers(int);
    void			cleanUp();

    virtual void		handleNewViewer(uiFlatViewer*)	{}
    void			makeInfoMsg(const IOPar&,uiString&);
};
