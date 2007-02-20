#ifndef uiflatviewer_h
#define uiflatviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewer.h,v 1.2 2007-02-20 12:04:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "flatdisp.h"
#include "uigeom.h"
namespace FlatDisp {
class BitMapMgr;
class BitMap2RGB;
}
class MouseEvent;
class uiRGBArray;
class uiRGBArrayCanvas;
class uiWorld2Ui;


/*!\brief Fulfills the FlatDisp::Viewer specifications using 'ui' classes. */

class uiFlatViewer : public uiGroup
		   , public FlatDisp::Viewer
{
public:
    			uiFlatViewer(uiParent*);
			~uiFlatViewer();
    void		initView(); //!< After you set the right PosData

    void		setExtraBorders(const uiSize& lt,const uiSize& rb);
    void		handleChange(DataChangeType);

    uiRGBArray&		rgbArray();
    uiRGBArrayCanvas&	rgbCanvas()			{ return canvas_; }

    void		setView(uiWorldRect);
    const uiWorldRect&	curView() const			{ return wr_; }
    uiWorldRect		boundingBox() const;
    void		getWorld2Ui(uiWorld2Ui&) const;
    void		setDim0ExtFac( float f )	{ dim0extfac_ = f; }
    			//!< when reporting boundingBox(), extends this
    			//!< amount of positions outward. Default 0.5.

    Notifier<uiFlatViewer> viewChanged; //!< i.e. zoom/pan, or setView

protected:

    uiRGBArrayCanvas&	canvas_;
    uiWorldRect		wr_;

    DataChangeType	reportedchange_;
    float		dim0extfac_;
    uiRect		extraborders_;
    uiSize		annotsz_;
    bool		anysetviewdone_;

    FlatDisp::BitMapMgr* wvabmpmgr_;
    FlatDisp::BitMapMgr* vdbmpmgr_;
    FlatDisp::BitMap2RGB* bmp2rgb_;

    void		canvasNewFill( CallBacker* )	{ drawBitMaps(); }
    void		canvasPostDraw(CallBacker*)	{ drawAnnot(); }
    uiWorldRect		getBoundingBox(bool) const;
    Color		color(bool foreground) const;

    void		drawBitMaps();
    void		drawAnnot();
    void		drawGridAnnot(bool);
    void		drawAux(const FlatDisp::Annotation::AuxData&);
};

#endif
