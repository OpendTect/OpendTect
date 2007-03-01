#ifndef uiflatviewcontrol_h
#define uiflatviewcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewcontrol.h,v 1.8 2007-03-01 19:35:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiFlatViewer;
class uiFlatViewPropDlg;
namespace FlatView { class ZoomMgr; }


/*!\brief Tools to control uiFlatViewer(s). */

class uiFlatViewControl : public uiGroup
{
public:

			~uiFlatViewControl();

    void		addViewer(uiFlatViewer&);
    			//!< No attaching done. Viewer may be in other window.

    uiWorldRect		getBoundingBox() const;
    void		setNewView(Geom::Point2D<double>& centre,
	    			   Geom::Size2D<double>& sizes);
    			//!< retains uiWorldRect's LR/TB swapping
    			//!< Changes the input to the actual new values
    void		flip(bool hor);
    			//!< reverses uiWorldRect's LR or TB swapping

    static bool		havePan(Geom::Point2D<double> oldcentre,
	    			Geom::Point2D<double> newcentre,
				Geom::Size2D<double> sz);
    static bool		haveZoom(Geom::Size2D<double> oldsz,
				 Geom::Size2D<double> newsz);

protected:

    			uiFlatViewControl(uiFlatViewer&,uiParent*,bool);

    ObjectSet<uiFlatViewer> vwrs_;
    FlatView::ZoomMgr&	zoommgr_;
    bool		haverubber_;

    uiFlatViewPropDlg*  propdlg_;

    uiWorldRect		getZoomAndPanRect(Geom::Point2D<double>,
	    				  Geom::Size2D<double>) const;
    uiWorldRect		getZoomOrPanRect(Geom::Point2D<double>,
	    				 Geom::Size2D<double>) const;

    virtual void	finalPrepare()			{}
    void		onFinalise(CallBacker*);
    void		rubBandCB(CallBacker*);

    void		doPropertiesDialog();
    void		propDlgClosed(CallBacker*);
    void		applyProperties(CallBacker* cb);
    void		saveProperties();

};

#endif
