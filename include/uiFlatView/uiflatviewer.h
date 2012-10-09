#ifndef uiflatviewer_h
#define uiflatviewer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id$
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
class uiGraphicsItemSet;
class uiArrowItem;
class uiEllipseItem;
class uiLineItem;
class uiMarkerItem;
class uiRectItem;
class uiTextItem;


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

    CNotifier<uiFlatViewer,uiWorldRect> viewChanging; //!< change thumbnail
    Notifier<uiFlatViewer> viewChanged; //!< setView
    Notifier<uiFlatViewer> dataChanged; //!< WVA or VD data changed
    Notifier<uiFlatViewer> dispParsChanged; //!< WVA or VD disppars changed

    uiFlatViewControl*	control()	{ return control_; }
    Interval<float>     getDataRange(bool) const;
    bool		drawBitMaps();
    bool		drawAnnot(const uiRect&,const uiWorldRect&);
    bool		drawAnnot();

    void		setNoViewDone()			
    			{ anysetviewdone_  = false; }
    bool		hasHandDrag() const		{ return enabhaddrag_; }
    void		setHandDrag( bool yn )		{ enabhaddrag_ = yn; }
    void		setViewBorder( const uiBorder& border )
    			{ viewborder_ = border; }
    uiBorder		annotBorder() const		{ return annotborder_; }
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
    void		disableReSizeDrawNotifier();
    void		showAuxDataObjects(FlatView::Annotation::AuxData&,bool);
    void		updateProperties(const FlatView::Annotation::AuxData&);
    void		reGenerate(FlatView::Annotation::AuxData&);
    void		remove(const FlatView::Annotation::AuxData&);

protected:

    uiRGBArrayCanvas&		canvas_;
    FlatView::AxesDrawer& 	axesdrawer_; //!< Must be declared after canvas_
    uiWorldRect			wr_;

    TypeSet<DataChangeType>	reportedchanges_;
    float			dim0extfac_;
    uiRect			extraborders_;
    uiBorder			annotborder_;
    uiBorder			viewborder_;
    uiSize			annotsz_;
    bool			initview_;
    bool			enabhaddrag_;
    bool			anysetviewdone_;
    bool			x0rev_;
    bool			x1rev_;
    bool			useseldataranges_;

    Interval<double>		xseldatarange_;
    Interval<double>		yseldatarange_;

    FlatView::BitMapMgr*	wvabmpmgr_;
    FlatView::BitMapMgr*	vdbmpmgr_;
    FlatView::BitMap2RGB*	bmp2rgb_;

    uiTextItem*			titletxtitem_;
    uiTextItem*			axis1nm_;
    uiTextItem*			axis2nm_;
    uiRectItem*			rectitem_;
    uiArrowItem*		arrowitem1_;
    uiArrowItem*		arrowitem2_;
    uiMarkerItem*		pointitem_;
    uiGraphicsItemSet*		polylineitemset_;
    uiGraphicsItemSet*		markeritemset_;
    uiGraphicsItemSet*		adnameitemset_;

    void			onFinalise(CallBacker*);
    void			reDrawAll();
    void			reSizeDraw(CallBacker*);
    uiWorldRect			getBoundingBox(bool) const;
    Color			color(bool foreground) const;

    void			drawGridAnnot(bool,const uiRect&,
	    				      const uiWorldRect&);
    void			drawAux(FlatView::Annotation::AuxData&,
	    				const uiRect&,const uiWorldRect&);

    void			reset();
    bool			mkBitmaps(uiPoint&);

    friend class		uiFlatViewControl;
    uiFlatViewControl*		control_;
};

#endif
