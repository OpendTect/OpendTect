#ifndef uiflatviewer_h
#define uiflatviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewer.h,v 1.14 2007-08-24 11:23:49 cvsbert Exp $
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


/*!\brief Fulfills the FlatView::Viewer specifications using 'ui' classes. */

class uiFlatViewer : public uiGroup
		   , public FlatView::Viewer
{
public:
    			uiFlatViewer(uiParent*);
			~uiFlatViewer();

    void		setDarkBG(bool);
    void		setExtraBorders(const uiSize& lt,const uiSize& rb);
    void		setInitialSize(uiSize);

    int			getAnnotChoices(BufferStringSet&) const;
    void		setAnnotChoice(int);

    uiRGBArray&		rgbArray();
    uiRGBArrayCanvas&	rgbCanvas()			{ return canvas_; }

    void		setView(uiWorldRect);
    const uiWorldRect&	curView() const			{ return wr_; }
    uiWorldRect		boundingBox() const;
    void		getWorld2Ui(uiWorld2Ui&) const;
    void		setDim0ExtFac( float f )	{ dim0extfac_ = f; }
    			//!< when reporting boundingBox(), extends this
    			//!< amount of positions outward. Default 0.5.

    void		handleChange(DataChangeType);

    Notifier<uiFlatViewer> viewChanged; //!< setView
    Notifier<uiFlatViewer> dataChanged; //!< WVA or VD data changed

    uiFlatViewControl*	control()	{ return control_; }

protected:

    uiRGBArrayCanvas&		canvas_;
    FlatView::AxesDrawer& 	axesdrawer_;
    				//!<Needs to be declared *after* canvas_
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

    void		canvasNewFill(CallBacker*);
    void		canvasPostDraw(CallBacker*);
    uiWorldRect		getBoundingBox(bool) const;
    Color		color(bool foreground) const;

    void		drawBitMaps();
    void		drawAnnot();
    void		drawGridAnnot();
    void		drawAux(const FlatView::Annotation::AuxData&);
    void		initView();

    friend class	uiFlatViewControl;
    uiFlatViewControl*	control_;
};

#endif
