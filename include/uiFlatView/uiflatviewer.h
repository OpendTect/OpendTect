#ifndef uiflatviewer_h
#define uiflatviewer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewer.h,v 1.57 2012-07-10 15:02:17 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uimainwin.h"
#include "flatview.h"
#include "uigeom.h"
#include "threadwork.h"

namespace FlatView {
class BitMapMgr;
class BitMap2RGB;
class AxesDrawer;
class uiAuxDataDisplay;
class uiBitMapDisplay;
}


class uiGraphicsView;
class BufferStringSet;
class uiRGBArrayCanvas;
class uiRGBArray;
class uiWorld2Ui;
class uiFlatViewControl;
class uiGraphicsItem;
class uiGraphicsItemSet;
class uiArrowItem;
class uiEllipseItem;
class uiLineItem;
class uiMarkerItem;
class uiRectItem;
class uiTextItem;
class uiGraphicsItemGroup;


/*!\brief Fulfills the FlatView::Viewer specifications using 'ui' classes. */

mClass uiFlatViewer : public uiGroup
		    , public FlatView::Viewer
{
public:
    			uiFlatViewer(uiParent*,bool enabhanddrag=false);
			~uiFlatViewer();

    void		setExtraBorders(const uiSize& lt,const uiSize& rb);
    void		setInitialSize(uiSize);

    int			getAnnotChoices(BufferStringSet&) const;
    void		setAnnotChoice(int);

    uiRGBArray&		rgbArray();
    uiGraphicsView&	rgbCanvas()			{ return *view_; }

    void		setView(const uiWorldRect&);
    const uiWorldRect&	curView() const			{ return wr_; }
    uiWorldRect		boundingBox() const;
    void		setViewToBoundingBox();

    void		getWorld2Ui(uiWorld2Ui&) const;
    void		setExtraBorders( uiRect r )	{ extraborders_ = r; }
    uiRect		getViewRect() const;
    			/*!<The rectangle onto which wr_ is projected */
    void		setRubberBandingOn(bool);
    void		setDim0ExtFac( float f )	{ dim0extfac_ = f; }
    			//!< when reporting boundingBox(), extends this
    			//!< amount of positions outward. Default 0.5.

    void		handleChange(DataChangeType,bool dofill = true);

    CNotifier<uiFlatViewer,uiWorldRect> viewChanging; //!< change thumbnail
    Notifier<uiFlatViewer> viewChanged; //!< setView
    Notifier<uiFlatViewer> dataChanged; //!< WVA or VD data changed

    uiFlatViewControl*	control()	{ return control_; }
    Interval<float>     getBitmapDataRange(bool) const;

    void		setNoViewDone()			
    			{ anysetviewdone_  = false; }
    bool		hasHandDrag() const		{ return enabhaddrag_; }
    void		setHandDrag( bool yn )		{ enabhaddrag_ = yn; }
    void		setViewBorder( const uiBorder& border )
    			{ viewborder_ = border; }
    
    uiBorder		viewBorder() const		{ return viewborder_; }
    uiBorder		getActBorder(const uiWorldRect&);
    static float	bufextendratio_;
    			//!< Must be > 0. default 0.4
    			//!< Controls how much extra bitmap is generated
    void		setUseSelDataRanges(bool yn) { useseldataranges_ = yn; }
    void		setSelDataRanges(Interval<double>,Interval<double>);
    			//restrain the data ranges between the selected ranges
    const Interval<double>& getSelDataRange(bool forx) const
    			{ return forx ? xseldatarange_ : yseldatarange_; } 
    void		disableReSizeDrawNotifier(){};
    void		reGenerate(FlatView::AuxData&);

    FlatView::AuxData*		createAuxData(const char* nm) const;
    int				nrAuxData() const;
    FlatView::AuxData*		getAuxData(int idx);
    const FlatView::AuxData*	getAuxData(int idx) const;
    void			addAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(FlatView::AuxData* a);
    FlatView::AuxData*		removeAuxData(int idx);

protected:

    uiGraphicsView*		view_;
    FlatView::AxesDrawer& 	axesdrawer_; //!< Must be declared after canvas_
    uiWorldRect			wr_;

    TypeSet<DataChangeType>	reportedchanges_;
    float			dim0extfac_;
    uiRect			extraborders_;
    uiBorder			viewborder_;
    bool			initview_;
    bool			enabhaddrag_;
    bool			anysetviewdone_;
    bool			x0rev_;
    bool			x1rev_;
    bool			useseldataranges_;

    Interval<double>		xseldatarange_;
    Interval<double>		yseldatarange_;

    FlatView::uiBitMapDisplay*	bitmapdisp_;

    uiTextItem*			titletxtitem_;
    uiTextItem*			axis1nm_;
    uiTextItem*			axis2nm_;
    uiRectItem*			rectitem_;
    uiArrowItem*		arrowitem1_;
    uiArrowItem*		arrowitem2_;
    uiMarkerItem*		pointitem_;
    uiGraphicsItemSet*		markeritemset_;
    uiGraphicsItemSet*		adnameitemset_;

    Threads::Work		annotwork_;
    Threads::Work		bitmapwork_;
    Threads::Work		auxdatawork_;

    void			updateCB(CallBacker*);
    void			updateAnnotCB(CallBacker*);
    void			updateBitmapCB(CallBacker*);
    void			updateAuxDataCB(CallBacker*);

    void			reSizeCB(CallBacker*);
    uiWorldRect			getBoundingBox(bool) const;
    Color			color(bool foreground) const;

    int				updatequeueid_;

    void			updateGridAnnot(bool visible);
    void			updateAuxData();
    bool			drawBitMaps();
    bool			drawAnnot();
    void			updateTransforms();
    void			updateAnnotPositions();
    void			updateAnnotation();

    bool			mkBitmaps(uiPoint&);

    friend class				uiFlatViewControl;
    uiGraphicsItemGroup*			worldgroup_;
    uiFlatViewControl*				control_;

    ObjectSet<FlatView::uiAuxDataDisplay>	auxdata_;
};

#endif
