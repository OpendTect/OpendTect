#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uiflatviewstdcontrol.h"
#include "flatview.h"

class TimeDepthModel;

/*!
\brief A control for flatviewers with different zoom properties and settings.
*/

mExpClass(uiFlatView) MFVCViewManager
{
public:
			MFVCViewManager()
			{}
			~MFVCViewManager();

    void		setD2TModels(const ObjectSet<const TimeDepthModel>&);
    void		setViewerType(const uiFlatViewer* vwr,bool isintime);
    bool		getViewRect(const uiFlatViewer* activevwr,
				    const uiFlatViewer* curvwr,
				    uiWorldRect&) const;
    void		setFlattened( bool flattened )
			{ isflattened_ = flattened; }
    bool		isFlattened() const	{ return isflattened_; }
protected:
    BoolTypeSet					zintimeflags_;
    ObjectSet<const TimeDepthModel>		d2tmodels_;
    ObjectSet<const uiFlatViewer>		vwrs_;
    bool					isflattened_ = false;
};


mExpClass(uiFlatView) uiMultiFlatViewControl : public uiFlatViewStdControl
{ mODTextTranslationClass(uiMultiFlatViewControl)
public:
			uiMultiFlatViewControl(uiFlatViewer&,const Setup&);
			~uiMultiFlatViewControl();

    void                setNewView(Geom::Point2D<double> mousepos,
				   Geom::Size2D<double> size,
				   uiFlatViewer* vwr=0);

    uiToolBar*		getToolBar(int idx) { return toolbars_[idx]; }

    uiFlatViewer*	activeVwr() const   { return activevwr_; }
    bool		setActiveVwr(int vwridx);

    void		setZoomCoupled( bool yn ) { iszoomcoupled_ = yn; }
    void		setDrawZoomBoxes( bool yn ) { drawzoomboxes_ = yn; }
    void		setViewerType( const uiFlatViewer* vwr, bool isintime )
			{ viewmgr_.setViewerType( vwr, isintime ); }
    void		setD2TModels(const ObjectSet<const TimeDepthModel>& d2t)
			{ viewmgr_.setD2TModels( d2t ); }
    void		setFlattened( bool flattened )
			{ viewmgr_.setFlattened( flattened ); }

    void		setParsButToolTip(const uiFlatViewer&,const uiString&);

protected:

    ObjectSet<uiToolBar> toolbars_;
    TypeSet<int> parsbuts_;
    ObjectSet<FlatView::AuxData> zoomboxes_;
    MFVCViewManager	viewmgr_;

    bool		handleUserClick(int vwridx);
    bool		iszoomcoupled_;
    bool		drawzoomboxes_;

    uiFlatViewer*	activevwr_;
    void		updateZoomManager();
			//!< Should be called after the viewer is zoomed in/out.

    void		rubBandCB(CallBacker*);
    void		parsCB(CallBacker*);
    void		setZoomAreasCB(CallBacker*);
    void		setZoomBoxesCB(CallBacker*);
    void		removeAnnotationsCB(CallBacker*);
    void		vwrAdded(CallBacker*);
    void		zoomCB(CallBacker*) override;
    void		wheelMoveCB(CallBacker*);
    void		pinchZoomCB(CallBacker*);
};
