#ifndef uifunctiondisplay_h
#define uifunctiondisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Apr 2008
 RCS:           $Id: uifunctiondisplay.h,v 1.4 2008-04-03 15:48:56 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicanvas.h"
#include "draw.h"
class uiAxisHandler;

/*!\brief displays a function of (X,Y) pairs on a canvas - optionally a Y2.

  * No undefined values supportd (yet). X values can't be undef anyway.
  * You cannot change Setup::editable_ after construction.
 
 */

class uiFunctionDisplay : public uiCanvas
{
public:

    struct Setup
    {
				Setup()
				    : xrg_(mUdf(float),mUdf(float))
				    , yrg_(mUdf(float),mUdf(float))
				    , y2rg_(mUdf(float),mUdf(float))
				    , bgcol_(Color::White)
				    , ycol_(0,0,150)
				    , y2col_(0,200,0)
				    , xmarkcol_(150,0,0)
				    , ymarkcol_(150,0,0)
				    , canvaswidth_(400)
				    , canvasheight_(250)
				    , border_(20,10,20,10)
				    , annoty_(true)
				    , pointsz_(0)
				    , editable_(false)
				    , fillbelow_(false)		{}

	mDefSetupMemb(Interval<float>,xrg)	//!< if fixed start or end
	mDefSetupMemb(Interval<float>,yrg)	//!< if fixed start or end
	mDefSetupMemb(Interval<float>,y2rg)	//!< if fixed start or end
	mDefSetupMemb(Color,bgcol)		//!< Canvas background
	mDefSetupMemb(Color,ycol)
	mDefSetupMemb(Color,y2col)
	mDefSetupMemb(Color,xmarkcol)
	mDefSetupMemb(Color,ymarkcol)
	mDefSetupMemb(int,canvaswidth)
	mDefSetupMemb(int,canvasheight)
	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(bool,annoty)
	mDefSetupMemb(int,pointsz)		//!< If > 0, points are drawn
	mDefSetupMemb(bool,editable)		//!< Add/remove/change Y1 pts
	mDefSetupMemb(bool,fillbelow)		//!< only Y1 will get fill
    };

				uiFunctionDisplay(uiParent*,const Setup&);
				~uiFunctionDisplay();

    void			setVals(const Interval<float>&,
	    				const float* yvals,int sz);
    void			setVals(const float* xvals,
	    				const float* yvals,int sz);
    void			setY2Vals(const float*);
    void			setMarkValue(float,bool is_x);

    const TypeSet<float>&	xVals() const	{ return xvals_; }
    const TypeSet<float>&	yVals() const	{ return yvals_; }
    uiAxisHandler*		xAxis()		{ return xax_; }
    uiAxisHandler*		yAxis( bool y2 ) { return y2?yax_:y2ax_; }
    Setup&			setup()		{ return setup_; }

    int				size() const	{ return xvals_.size(); }

    Notifier<uiFunctionDisplay>	pointSelected;
    Notifier<uiFunctionDisplay>	pointChanged;
    int				selPt() const	{ return selpt_; }


protected:

    Setup			setup_;
    uiAxisHandler*		xax_;
    uiAxisHandler*		yax_;
    uiAxisHandler*		y2ax_;
    TypeSet<float>		xvals_;
    TypeSet<float>		yvals_;
    TypeSet<float>		y2vals_;
    float			xmarkval_;
    float			ymarkval_;
    int				selpt_;
    bool			mousedown_;

    void			gatherInfo();
    virtual void		reDrawHandler(uiRect);

    void			mousePress(CallBacker*);
    void			mouseRelease(CallBacker*);
    void			mouseMove(CallBacker*);
    void			mouseDClick(CallBacker*);

    bool			setSelPt();
};


#endif
