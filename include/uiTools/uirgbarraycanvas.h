#ifndef uirgbarraycanvas_h
#define uirgbarraycanvas_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uirgbarraycanvas.h,v 1.13 2012-08-03 13:01:14 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsview.h"
#include "color.h"

class uiPixmapItem;
class uiRGBArray;
class ioPixmap;
class MouseEventHandler;


/*!\brief Provides a canvas where a uiRGBArray is pre-drawn.

  Handles borders, drawing and clearing.
  When the preDraw is triggered, the usedArea() is known, and the uiRGBArray
  was resized and cleared to the background color. You can choose to
  implement your own mkNewFill() function if you want to subclass, or catch
  the newFillNeeded notifier (if both: newFillNeeded is done first).
  At postDraw you typically put annotations and so forth, they are only
  needed in the updateArea().

 */

mClass(uiTools) uiRGBArrayCanvas : public uiGraphicsView
{
public:
    			uiRGBArrayCanvas(uiParent*,uiRGBArray&);
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

    void		beforeDraw();
    void		beforeDraw(int w,int h);
    void 		setPixMapPos(int x,int y);
    void		draw();

protected:

    uiPixmapItem*	pixmapitm_;
    uiRGBArray&		rgbarr_;
    uiBorder		border_;
    Color		bgcolor_;
    bool		dodraw_;

    uiRect		arrarea_;
    uiRect		updarea_;
    ioPixmap*		pixmap_;

    virtual void	mkNewFill()		{}
    void		rubberBandHandler(uiRect);
    bool		createPixmap();

};


#endif

