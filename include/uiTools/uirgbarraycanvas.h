#ifndef uirgbarraycanvas_h
#define uirgbarraycanvas_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uirgbarraycanvas.h,v 1.3 2008-03-04 11:56:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicanvas.h"
#include "color.h"
class uiRGBArray;
class ioPixmap;


/*!\brief Provides a canvas where a uiRGBArray is pre-drawn.

  Handles borders, drawing and clearing.
  When the preDraw is triggered, the usedArea() is known, and the uiRGBArray
  was resized and cleared to the background color. You can choose to
  implement your own mkNewFill() function if you want to subclass, or catch
  the newFillNeeded notifier (if both: newFillNeeded is done first).
  At postDraw you typically put annotations and so forth, they are only
  needed in the updateArea().

 */

class uiRGBArrayCanvas : public uiCanvas
{
public:
    			uiRGBArrayCanvas(uiParent*,uiRGBArray&);

    void		setBorder(const uiBorder&);
    void		setBGColor(const Color&); //!< everything
    void		setDrawArr(bool);	//!< Draw the arr or not?

    uiRect		arrArea() const		{ return arrarea_; }
    uiRGBArray&		rgbArray()		{ return rgbarr_; }
    const uiRGBArray&	rgbArray() const	{ return rgbarr_; }

    void		forceNewFill();

    const uiRect&	updateArea()		{ return updarea_; }
    			//!< In this area the 'rest' needs to be drawn
    			//!< in your own reDrawHandler or at postDraw

    inline const uiBorder& border() const	{ return border_; }
    inline const Color&	bgColor() const		{ return bgcolor_; }
    inline bool		arrDrawn() const	{ return dodraw_; }

    Notifier<uiRGBArrayCanvas>	newFillNeeded;
    Notifier<uiRGBArrayCanvas>	rubberBandUsed;	// CallBacker: CBCapsule<uiRect>
        
protected:

    uiRGBArray&		rgbarr_;
    uiBorder		border_;
    Color		bgcolor_;
    bool		dodraw_;

    uiRect		arrarea_;
    uiRect		updarea_;
    ioPixmap*		pixmap_;

    virtual void	mkNewFill()		{}
    void		setupChg();
    void		beforeDraw(CallBacker*);
    void		reDrawHandler(uiRect);
    void		rubberBandHandler(uiRect);
    bool		createPixmap();

};


#endif
