#ifndef uiflatviewer_h
#define uiflatviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewer.h,v 1.30 2009-05-27 04:35:19 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uimainwin.h"
#include "flatview.h"
#include "uigeom.h"
namespace FlatView {
class BitMapMgr;
class BitMap2RGB;
class AxesDrawer;
}
class BufferStringSet;
class uiRGBArrayCanvas;
class uiRGBArray;
class uiWorld2Ui;
class uiFlatViewControl;
class uiGraphicsItem;
class uiGraphicsItemGroup;
class uiArrowItem;
class uiEllipseItem;
class uiLineItem;
class uiMarkerItem;
class uiPolygonItem;
class uiRectItem;
class uiTextItem;


/*!\brief Fulfills the FlatView::Viewer specifications using 'ui' classes. */

mClass uiFlatViewer : public uiGroup
		   , public FlatView::Viewer
{
public:
    			uiFlatViewer(uiParent*);
			~uiFlatViewer();

    void		setExtraBorders(const uiSize& lt,const uiSize& rb);
    void		setInitialSize(uiSize);

    int			getAnnotChoices(BufferStringSet&) const;
    void		setAnnotChoice(int);

    uiRGBArray&		rgbArray();
    uiRGBArrayCanvas&	rgbCanvas()			{ return canvas_; }

    void		setView(const uiWorldRect&);
    const uiWorldRect&	curView() const			{ return wr_; }
    uiWorldRect		boundingBox() const;
    void		getWorld2Ui(uiWorld2Ui&) const;
    void		setExtraBorders( uiRect r )	{ extraborders_ = r; }
    void		setRubberBandingOn(bool);
    void		setDim0ExtFac( float f )	{ dim0extfac_ = f; }
    			//!< when reporting boundingBox(), extends this
    			//!< amount of positions outward. Default 0.5.

    void		handleChange(DataChangeType,bool dofill = true);

    Notifier<uiFlatViewer> viewChanged; //!< setView
    Notifier<uiFlatViewer> dataChanged; //!< WVA or VD data changed
    Notifier<uiFlatViewer> dispParsChanged; //!< WVA or VD disppars changed

    uiFlatViewControl*	control()	{ return control_; }
    Interval<float>     getDataRange(bool) const;
    void		drawBitMaps();
    void		drawAnnot();


    static float	bufextendratio_;
    			//!< Must be > 0. default 0.4
    			//!< Controls how much extra bitmap is generated

protected:

    uiRGBArrayCanvas&		canvas_;
    FlatView::AxesDrawer& 	axesdrawer_; //!< Must be declared after canvas_
    uiWorldRect			wr_;

    TypeSet<DataChangeType>	reportedchanges_;
    float			dim0extfac_;
    uiRect			extraborders_;
    uiSize			annotsz_;
    uiWorldRect			prevwr_;
    uiSize			prevsz_;
    bool			anysetviewdone_;
    bool			x0rev_;
    bool			x1rev_;

    FlatView::BitMapMgr*	wvabmpmgr_;
    FlatView::BitMapMgr*	vdbmpmgr_;
    FlatView::BitMap2RGB*	bmp2rgb_;

    uiTextItem*			titletxtitem_;
    uiTextItem*			axis1nm_;
    uiTextItem*			axis2nm_;
    uiTextItem*			addatanm_;
    uiRectItem*			rectitem_;
    uiArrowItem*		arrowitem1_;
    uiArrowItem*		arrowitem2_;
    uiPolygonItem*		polyitem_;
    uiGraphicsItemGroup*	polylineitmgrp_;
    uiGraphicsItemGroup*	markeritemgrp_;
    void			onFinalise(CallBacker*);
    void			reDraw(CallBacker*);
    uiWorldRect			getBoundingBox(bool) const;
    Color			color(bool foreground) const;

    void			drawGridAnnot(bool);
    void			drawAux(const FlatView::Annotation::AuxData&);

    void			reset();
    bool			mkBitmaps(uiPoint&);

    friend class		uiFlatViewControl;
    uiFlatViewControl*		control_;
};

#endif
