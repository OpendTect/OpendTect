#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			MFVCViewManager();
			~MFVCViewManager();

    void		setD2TModels(const ObjectSet<const TimeDepthModel>&);
    void		setViewerType(const uiFlatViewer* vwr,bool isintime);
    bool		getViewRect(const uiFlatViewer* activevwr,
				    const uiFlatViewer* curvwr,
				    uiWorldRect&) const;
    void		setFlattened( bool flattened )
			{ isflattened_ = flattened; }
    bool		isFlattened() const	{ return isflattened_; }

    void		setDepthShift(float);
    void		setZFactor(float);

protected:
    BoolTypeSet					zintimeflags_;
    ObjectSet<const TimeDepthModel>		d2tmodels_;
    float					zshift_ = 0.f;
    float					zfactor_ = 1.f;
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
				   uiFlatViewer* vwr=0) override;

    uiToolBar*		getToolBar(int idx) { return toolbars_[idx]; }

    uiFlatViewer*	activeVwr() const   { return activevwr_; }
    bool		setActiveVwr(int vwridx);

    void		setZoomCoupled( bool yn ) { iszoomcoupled_ = yn; }
    void		setDrawZoomBoxes( bool yn ) { drawzoomboxes_ = yn; }
    void		setViewerType( const uiFlatViewer* vwr, bool isintime )
			{ viewmgr_.setViewerType( vwr, isintime ); }
    void		setD2TModels(const ObjectSet<const TimeDepthModel>& d2t)
			{ viewmgr_.setD2TModels( d2t ); }
    void		setDepthShift( float shift )
			{ viewmgr_.setDepthShift( shift ); }
    void		setZFactor( float scale )
			{ viewmgr_.setZFactor( scale ); }
    void		setFlattened( bool flattened )
			{ viewmgr_.setFlattened( flattened ); }

    void		setParsButToolTip(const uiFlatViewer&,const uiString&);

protected:

    ObjectSet<uiToolBar> toolbars_;
    TypeSet<int> parsbuts_;
    ObjectSet<FlatView::AuxData> zoomboxes_;
    MFVCViewManager	viewmgr_;

    bool		handleUserClick(int vwridx) override;
    bool		iszoomcoupled_;
    bool		drawzoomboxes_;

    uiFlatViewer*	activevwr_;
    void		updateZoomManager() override;
			//!< Should be called after the viewer is zoomed in/out.

    void		rubBandCB(CallBacker*) override;
    void		parsCB(CallBacker*) override;
    void		setZoomAreasCB(CallBacker*);
    void		setZoomBoxesCB(CallBacker*);
    void		removeAnnotationsCB(CallBacker*);
    void		vwrAdded(CallBacker*) override;
    void		zoomCB(CallBacker*) override;
    void		wheelMoveCB(CallBacker*) override;
    void		pinchZoomCB(CallBacker*) override;
};
