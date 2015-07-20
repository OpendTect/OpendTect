#ifndef uimultiflatviewcontrol_h
#define uimultiflatviewcontrol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uiflatviewstdcontrol.h"
#include "flatview.h"
#include "velocitycalc.h"

/*!
\brief A control for flatviewers with different zoom properties and settings.
*/

mExpClass(uiFlatView) MFVCViewManager
{
public:
    void		setD2TModels(const ObjectSet<const TimeDepthModel>&);
    void		setViewerType(const uiFlatViewer* vwr,bool isintime);
    bool		getViewRect(const uiFlatViewer* activevwr,
				    const uiFlatViewer* curvwr,
				    uiWorldRect&) const;
protected:
    BoolTypeSet					zintimeflags_;
    ObjectSet<const TimeDepthModel>		d2tmodels_;
    ObjectSet<const uiFlatViewer>		vwrs_;

public:
			~MFVCViewManager();
			MFVCViewManager();
    void		setFlattened(bool flattened);
    bool		isFlattened() const;
};


mExpClass(uiFlatView) uiMultiFlatViewControl : public uiFlatViewStdControl
{ mODTextTranslationClass(uiMultiFlatViewControl)
public:
    			uiMultiFlatViewControl(uiFlatViewer&,const Setup&);
    			~uiMultiFlatViewControl();

    void                setNewView(Geom::Point2D<double>& centre,
	                                       Geom::Size2D<double>& sizes);

    uiToolBar*		getToolBar(int idx) { return toolbars_[idx]; }

    uiFlatViewer*	activeVwr() const   { return activevwr_; }
    bool		setActiveVwr(int vwridx);

    void		setZoomCoupled( bool yn ) { iszoomcoupled_ = yn; }
    void		setDrawZoomBoxes( bool yn ) { drawzoomboxes_ = yn; }
    void		setViewerType(const uiFlatViewer*, bool isintime);
    void		setD2TModels(const ObjectSet<const TimeDepthModel>&);

    void		reInitZooms();

protected:

    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiToolButton> parsbuts_;
    ObjectSet<FlatView::AuxData> zoomboxes_;

    bool		handleUserClick(int vwridx);
    bool		iszoomcoupled_;
    bool		drawzoomboxes_;

    uiFlatViewer*	activevwr_;

    void		rubBandCB(CallBacker*);
    void		parsCB(CallBacker*);
    void		setZoomAreasCB(CallBacker*);
    void		setZoomBoxesCB(CallBacker*);
    void		vwrAdded(CallBacker*);
    void		zoomCB(CallBacker*);
    void		wheelMoveCB(CallBacker*);
    void		pinchZoomCB(CallBacker*);
public:
    uiToolButton*	parsButton(const uiFlatViewer*);
    void		setFlattened(bool flattened);

};

#endif

