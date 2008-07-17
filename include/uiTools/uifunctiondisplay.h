#ifndef uifunctiondisplay_h
#define uifunctiondisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Apr 2008
 RCS:           $Id: uifunctiondisplay.h,v 1.9 2008-07-17 11:53:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicanvas.h"
#include "draw.h"
class uiAxisHandler;

/*!\brief displays a function of (X,Y) pairs on a canvas - optionally a Y2.

  * No undefined values supportd (yet). X values can't be undef anyway.
  * You cannot change Setup::editable_ after construction.
  * Y2 is just an optional annotation. It can not be edited, it does not
    determine the X axis scale. Also, no markers are drawn.
 
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
				    , annotx_(true)
				    , annoty_(true)
				    , pointsz_(0)
				    , ptsnaptol_(0.01)
				    , editable_(false)
				    , fillbelow_(false)
				    , fillbelowy2_(false)	{}

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
	mDefSetupMemb(bool,annotx)
	mDefSetupMemb(bool,annoty)
	mDefSetupMemb(int,pointsz)		//!< If > 0, points are drawn
	mDefSetupMemb(bool,editable)		//!< Add/remove/change Y1 pts
	mDefSetupMemb(bool,fillbelow)		//!< Y1 will get fill
	mDefSetupMemb(bool,fillbelowy2)		//!< Y2 will get fill
	mDefSetupMemb(float,ptsnaptol)		//!< Snap tol ratio of axis size
    };

				uiFunctionDisplay(uiParent*,const Setup&);
				~uiFunctionDisplay();

    void			setVals(const float* xvals,
	    				const float* yvals,int sz);
    void			setVals(const Interval<float>&,
	    				const float* yvals,int sz);
    void			setY2Vals(const float* xvals,
	    				  const float* yvals,int sz);
    void			setMarkValue(float,bool is_x);

    const TypeSet<float>&	xVals() const	{ return xvals_; }
    const TypeSet<float>&	yVals() const	{ return yvals_; }
    uiAxisHandler*		xAxis()		{ return xax_; }
    uiAxisHandler*		yAxis( bool y2) { return y2 ? y2ax_ : yax_; }
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
    TypeSet<float>		y2yvals_;
    TypeSet<float>		y2xvals_;
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
    void			getRanges(const TypeSet<float>&,
	    				  const TypeSet<float>&,
					  const Interval<float>&,
					  const Interval<float>&,
					  StepInterval<float>&,
					  StepInterval<float>&) const;
};


#endif
