#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigroup.h"
#include "iopar.h"
#include "flatviewzoommgr.h"

class uiFlatViewer;
class uiFlatViewPropDlg;
class uiTabStackDlg;
class MouseEventHandler;
namespace FlatView { class Viewer; }

/*!
\brief Tools to control uiFlatViewer(s).
*/

mExpClass(uiFlatView) uiFlatViewControl : public uiGroup
{
public:

			~uiFlatViewControl();

    void		addViewer(uiFlatViewer&);
			//!< No attaching done. Viewer may be in other window.
    void		removeViewer(uiFlatViewer&);

    TypeSet<uiWorldRect>getBoundingBoxes() const;
			//!< Returns bounding boxes of all viewers.
    virtual void	setNewView(Geom::Point2D<double> mousepos,
				   Geom::Size2D<double> newsize,
				   uiFlatViewer* vwr=0);
			/*!< Pass centre instead of mousepos if there is no
			MouseEvent. Retains uiWorldRect's LR/TB swapping while
			changing the input to the actual new values. Use for
			setting new view while zoomin/zoom out only. Makes sure
			\param mousepos (pointed by MouseCursor) does not change
			after changing view and bitmaps are filled in extra
			available space without changing aspect ratio
			(if has to be constant) along with needed
			uiFlatViewer::setBoundingRect(const uiRect&).
			\param newsize
			\param vwr
			 */
    virtual void	flip(bool hor);
			//!< reverses uiWorldRect's LR or TB swapping
    virtual void	doPropertiesDialog(int vieweridx=0);

    int			getViewerIdx(const MouseEventHandler*,bool ofscene);
			//!< ofscene should be true while passing
			//!< MouseEventHandler of uiGraphicsScene and false
			//!< while passing MouseEventHandler of uiGraphicsView.

    Notifier<uiFlatViewControl>  infoChanged;	// CallBacker: CBCapsule<IOPar>
    Notifier<uiFlatViewControl>  viewerAdded;
    Notifier<uiFlatViewControl>  zoomChanged;
    Notifier<uiFlatViewControl>  rubberBandUsed;

    static uiWorldRect		getZoomOrPanRect(Geom::Point2D<double> mousepos,
						 Geom::Size2D<double> newsz,
						 const uiWorldRect& view,
						 const uiWorldRect& bbox);
				/*!< If size of view and newsz differ,
				zooms in/out the uiWorldRect represented by
				view (current). Returns the resulting
				uiWorldRect after shifting it such that it falls
				inside the bounding box. Pass centre instead of
				mousepos if there is no MouseEvent. In case of
				panning, pass the view after translation. */
    const FlatView::ZoomMgr&	zoomMgr() const { return zoommgr_; }
    void			reInitZooms();

protected:

			uiFlatViewControl(uiFlatViewer&,uiParent*,bool);

    ObjectSet<uiFlatViewer> vwrs_;
    FlatView::ZoomMgr	zoommgr_;
    bool		haverubber_;
    IOPar		infopars_;
    bool	       initdone_ = false;

    uiFlatViewPropDlg*	propdlg_ = nullptr;

    MouseEventHandler&	mouseEventHandler(int vwridx,bool ofscene);
			//!< Returns MouseEventHandler of uiGraphicsScene if
			//!< ofscene is true else returns that of uiGraphicsView

    virtual void	finalPrepare()			{}
    virtual void	onFinalize(CallBacker*);
    virtual bool	canReUseZoomSettings( Geom::Point2D<double>,
					      Geom::Size2D<double> ) const;
    virtual void	setViewToCustomZoomLevel(uiFlatViewer&) {}
    virtual void	setNewWorldRect(uiFlatViewer&,uiWorldRect&);
			/*!< Sets uiWorldRect that can be filled in
			available space without changing aspect ratio along with
			needed uiFlatViewer::setBoundingRect(const uiRect&). */
    virtual void	updateZoomManager();

    virtual void	dataChangeCB(CallBacker*);
    virtual void	rubBandCB(CallBacker*);
    virtual void	vwrAdded(CallBacker*)	{}
    virtual void	handDragStarted(CallBacker*)	{}
    virtual void	handDragging(CallBacker*)	{}
    virtual void	handDragged(CallBacker*)	{}
    virtual void	mouseMoveCB(CallBacker*);
    virtual void	keyPressCB(CallBacker*)		{}
    virtual void	usrClickCB(CallBacker*);
    virtual bool	handleUserClick(int vwridx)	{ return false; }

    virtual void	propDlgClosed(CallBacker*);
    virtual void	applyProperties(CallBacker* cb);
    virtual void	saveProperties(FlatView::Viewer&);

    void		initZoom(CallBacker*);

};

