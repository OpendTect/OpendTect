#ifndef uirgbarraycanvas_h
#define uirgbarraycanvas_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uirgbarraycanvas.h,v 1.9 2009-04-15 12:13:22 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "pixmap.h"
#include "color.h"
class uiPixmapItem;
class MouseEventHandler;
class uiRGBArray;


/*!\brief Provides a canvas where a uiRGBArray is pre-drawn.

  Handles borders, drawing and clearing.
  When the preDraw is triggered, the usedArea() is known, and the uiRGBArray
  was resized and cleared to the background color. You can choose to
  implement your own mkNewFill() function if you want to subclass, or catch
  the newFillNeeded notifier (if both: newFillNeeded is done first).
  At postDraw you typically put annotations and so forth, they are only
  needed in the updateArea().

 */

mClass uiRGBArrayCanvas : public uiGraphicsView
{
public:
    mClass Setup
    {
    public:
			Setup( bool sb = true, bool hd = true,int w=0, int h=0 )
			    : scrollbar_(sb)
			    , withsaveimage_(false)
			    , width_(w)
			    , height_(h)	{}
	mDefSetupMemb	(bool,scrollbar)
	mDefSetupMemb	(bool,withsaveimage)
	mDefSetupMemb	(int,width)
	mDefSetupMemb	(int,height)
    };
    			uiRGBArrayCanvas(uiParent*,
					 const uiRGBArrayCanvas::Setup&,
					 uiRGBArray&);
			~uiRGBArrayCanvas();

    void		setBorder(const uiBorder&);
    void		setBGColor(const Color&); //!< everything
    void		setDrawArr(bool);	//!< Draw the arr or not?
    void		setPixmap(const ioPixmap&);
    void		removePixmap();

    uiRect		arrArea() const		{ return arrarea_; }
    uiRGBArray&		rgbArray()		{ return rgbarr_; }
    const uiRGBArray&	rgbArray() const	{ return rgbarr_; }

    const uiRect&	updateArea()		{ return updarea_; }
    			//!< In this area the 'rest' needs to be drawn
    			//!< in your own reDrawHandler or at postDraw

    inline const uiBorder& border() const	{ return border_; }
    inline const Color&	bgColor() const		{ return bgcolor_; }
    inline bool		arrDrawn() const	{ return dodraw_; }

    void			setBackgroundQpaque(bool);
    void			setWidth( int w ) 	{ width_ = w ; } 
    void			setHeight( int h )	{ height_ = h ; }
    void			beforeDraw();
    void 			setPixMapPos(int x,int y);
    void			draw();

        
protected:

    uiPixmapItem*	pixmapitm_;
    uiRGBArray&		rgbarr_;
    uiBorder		border_;
    Color		bgcolor_;
    bool		dodraw_;
    int			width_;
    int			height_;

    uiRect		arrarea_;
    uiRect		updarea_;
    ioPixmap*		pixmap_;

    virtual void	mkNewFill()		{}
    void		rubberBandHandler(uiRect);
    bool		createPixmap();

};


#endif
