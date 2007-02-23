#ifndef uiflatviewcontrol_h
#define uiflatviewcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewcontrol.h,v 1.3 2007-02-23 09:35:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiToolButton;
class uiButtonGroup;
class uiFlatViewer;
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

    void		setNewView(Geom::Point2D<double> centre,
	    			   Geom::Size2D<double> radius);
    			//!< retains uiWorldRect's LR/TB swapping

protected:

    ObjectSet<uiFlatViewer> vwrs_;
    const Setup&	setup_;
    FlatView::ZoomMgr&	zoommgr_;

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

    void		updatePosButtonStates();

    void		initStates(CallBacker*);
    void		zoomCB(CallBacker*);
    void		panCB(CallBacker*);
    void		stateCB(CallBacker*);
    void		parsCB(CallBacker*);

};

#endif
