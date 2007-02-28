#ifndef uiflatviewcontrol_h
#define uiflatviewcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewcontrol.h,v 1.6 2007-02-28 19:05:41 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiToolButton;
class uiButtonGroup;
class uiFlatViewer;
class uiFlatViewPropDlg;
namespace FlatView { class ZoomMgr; }


/*!\brief Tools to control uiFlatViewer(s). */

class uiFlatViewControl : public uiGroup
{
public:

    struct Setup
    {
			Setup( bool vert=true )
			    : vertical_(vert)
			    , withstates_(true)		{}

	mDefSetupMemb(bool,	vertical)
	mDefSetupMemb(bool,	withstates)
    };

    			uiFlatViewControl(uiFlatViewer&,const Setup&);
			~uiFlatViewControl();

    void		addViewer(uiFlatViewer&);
    			//!< No attaching done. Viewer may be in other window.

    void		setNewView(Geom::Point2D<double>& centre,
	    			   Geom::Size2D<double>& sizes);
    			//!< retains uiWorldRect's LR/TB swapping
    			//!< Changes the input to the actual new values
    void		flip(bool hor);
    			//!< reverses uiWorldRect's LR or TB swapping

protected:

    ObjectSet<uiFlatViewer> vwrs_;
    const Setup&	setup_;
    FlatView::ZoomMgr&	zoommgr_;

    uiFlatViewPropDlg*  propdlg_;

    uiButtonGroup*	posgrp_;
    uiButtonGroup*	stategrp_;
    uiButtonGroup*	parsgrp_;

    uiToolButton*	zoominbut_;
    uiToolButton*	zoomoutbut_;
    uiToolButton*	panupbut_;
    uiToolButton*	panleftbut_;
    uiToolButton*	panrightbut_;
    uiToolButton*	pandownbut_;
    uiToolButton*	manipbut_;
    uiToolButton*	drawbut_;
    uiToolButton*	parsbut_;

    uiWorldRect		getBoundingBox() const;
    void		updatePosButtonStates();

    void		initStates(CallBacker*);
    void		zoomCB(CallBacker*);
    void		panCB(CallBacker*);
    void		flipCB(CallBacker*);
    void		stateCB(CallBacker*);
    void		parsCB(CallBacker*);
    void		rubBandCB(CallBacker*);

    void		propDlgClosed(CallBacker*);
    void		applyProperties(CallBacker* cb);
    void		saveProperties();

};

#endif
