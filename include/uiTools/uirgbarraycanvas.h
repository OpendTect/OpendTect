#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsview.h"
#include "color.h"

class uiPixmapItem;
class uiRGBArray;
class uiPixmap;
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

mExpClass(uiTools) uiRGBArrayCanvas : public uiGraphicsView
{
public:
    			uiRGBArrayCanvas(uiParent*,uiRGBArray&);
			~uiRGBArrayCanvas();

    void		setBorder(const uiBorder&);
    void		setBGColor(const OD::Color&); //!< everything
    void		setDrawArr(bool);	//!< Draw the arr or not?
    void		setPixmap(const uiPixmap&);
    void		removePixmap();

    uiRect		arrArea() const		{ return arrarea_; }
    uiRGBArray&		rgbArray()		{ return rgbarr_; }
    const uiRGBArray&	rgbArray() const	{ return rgbarr_; }

    const uiRect&	updateArea()		{ return updarea_; }
    			//!< In this area the 'rest' needs to be drawn
    			//!< in your own reDrawHandler or at postDraw

    inline const uiBorder&  border() const	{ return border_; }
    inline const OD::Color& bgColor() const	{ return bgcolor_; }
    inline bool		arrDrawn() const	{ return dodraw_; }

    void		beforeDraw();
    void		beforeDraw(int w,int h);
    void 		setPixMapPos(int x,int y);
    void		updatePixmap();

protected:

    uiPixmapItem*	pixmapitm_;
    uiRGBArray&		rgbarr_;
    uiBorder		border_;
    OD::Color		bgcolor_;
    bool		dodraw_;

    uiRect		arrarea_;
    uiRect		updarea_;
    uiPixmap*		pixmap_;

    virtual void	mkNewFill()		{}
    void		rubberBandHandler(uiRect);
    bool		createPixmap();

};
