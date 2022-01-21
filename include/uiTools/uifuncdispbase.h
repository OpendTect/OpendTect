#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Jan 2022
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "draw.h"
#include "uigeom.h"

class uiAxisHandlerBase;
class uiObject;
class NotifierAccess;

/*!\brief base class for function displays.

 */

mExpClass(uiTools) uiFuncDispBase
{
public:

    struct Setup
    {
				Setup()
				    : xrg_(mUdf(float),mUdf(float))
				    , yrg_(mUdf(float),mUdf(float))
				    , y2rg_(mUdf(float),mUdf(float))
				    , bgcol_(OD::Color::White())
				    , ycol_(200,160,140)
				    , y2col_(140,160,200)
				    , ywidth_(2)
				    , y2width_(2)
				    , canvaswidth_(400)
				    , canvasheight_(250)
				    , border_(20,20,20,10)
				    , annotx_(true)
				    , annoty_(true)
				    , annoty2_(true)
				    , noxaxis_(false)
				    , noyaxis_(false)
				    , noy2axis_(false)
				    , noxgridline_(false)
				    , noygridline_(false)
				    , noy2gridline_(false)
				    , pointsz_(0)
				    , ptsnaptol_(0.01)
				    , editable_(false)
				    , curvzvaly_(1)
				    , curvzvaly2_(0)
				    , fillbelow_(false)
				    , fillbelowy2_(false)
				    , drawscattery1_(false)
				    , drawscattery2_(false)
				    , markerstyley1_(MarkerStyle2D::Square,3,
						     ycol_)
				    , markerstyley2_(MarkerStyle2D::Square,3,
						     y2col_)
				    , markerfilly1_(false)
				    , markerfilly2_(false)
				    , drawborder_(false)
				    , fixdrawrg_(true)
				    , borderstyle_(OD::LineStyle())
				    , closepolygon_(true)
				    , drawliney_(true)
				    , useyscalefory2_(false)
				    , xannotinint_(false)
				    , yannotinint_(false)
				    , drawliney2_(true) {}

	mDefSetupMemb(Interval<float>,xrg)	//!< if fixed start or end
	mDefSetupMemb(Interval<float>,yrg)	//!< if fixed start or end
	mDefSetupMemb(Interval<float>,y2rg)	//!< if fixed start or end
	mDefSetupMemb(OD::Color,bgcol)		//!< Canvas background
	mDefSetupMemb(OD::Color,ycol)
	mDefSetupMemb(OD::Color,y2col)
	mDefSetupMemb(int,ywidth)
	mDefSetupMemb(int,y2width)
	mDefSetupMemb(int,canvaswidth)
	mDefSetupMemb(int,canvasheight)
	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(int,curvzvaly)
	mDefSetupMemb(int,curvzvaly2)
	mDefSetupMemb(bool,annotx)
	mDefSetupMemb(bool,annoty)
	mDefSetupMemb(bool,annoty2)
	mDefSetupMemb(bool,noxaxis)
	mDefSetupMemb(bool,noyaxis)
	mDefSetupMemb(bool,noy2axis)
	mDefSetupMemb(bool,noxgridline)
	mDefSetupMemb(bool,noygridline)
	mDefSetupMemb(bool,noy2gridline)
	mDefSetupMemb(int,pointsz)		//!< If > 0, points are drawn
	mDefSetupMemb(bool,editable)		//!< Add/remove/change Y1 pts
	mDefSetupMemb(bool,fillbelow)		//!< Y1 will get fill
	mDefSetupMemb(bool,fillbelowy2)		//!< Y2 will get fill
	mDefSetupMemb(bool,drawliney)		//!< Y1 will be polylines
	mDefSetupMemb(bool,drawliney2)		//!< Y2 will be polylines
	mDefSetupMemb(bool,drawscattery1)		//!< draw Markers
	mDefSetupMemb(bool,drawscattery2)		//!< draw Markers
	mDefSetupMemb(MarkerStyle2D,markerstyley1)
	mDefSetupMemb(MarkerStyle2D,markerstyley2)
	mDefSetupMemb(bool,markerfilly1)
	mDefSetupMemb(bool,markerfilly2)
	mDefSetupMemb(bool,closepolygon)
	mDefSetupMemb(bool,drawborder)
	mDefSetupMemb(bool,useyscalefory2)
	mDefSetupMemb(OD::LineStyle,borderstyle)
	mDefSetupMemb(float,ptsnaptol)		//!< Snap tol ratio of axis size
	mDefSetupMemb(bool,fixdrawrg)
	mDefSetupMemb(bool,xannotinint)
	mDefSetupMemb(bool,yannotinint)

	Setup&		drawline( bool yn )
			{ drawliney_ = drawliney2_ = yn; return *this; }
	Setup&		drawgridlines( bool yn )
			{ noxgridline_ = noygridline_ = noy2gridline_ = !yn;
			  return *this; }
	Setup&		drawscatter( bool yn )
			{ drawscattery1_ = drawscattery2_ = yn; return *this; }
	Setup&		annot( bool yn )
			{ annotx_ = annoty_ = annoty2_ = yn; return *this; }
	Setup&		axes( bool yn )
			{ noxaxis_ = noyaxis_ = noy2axis_ = !yn; return *this; }

    };

				uiFuncDispBase(const Setup& su)
				    : setup_(su)
				    , xmarklineval_(mUdf(float))
				    , ymarklineval_(mUdf(float))
				    , xmarkline2val_(mUdf(float))
				    , ymarkline2val_(mUdf(float))
				    , selpt_(0) {}

    virtual			~uiFuncDispBase();

    Setup&			setup()		{ return setup_; }

    virtual void		setVals(const float* xvals,
					const float* yvals,int sz);
    virtual void		setVals(const Interval<float>&,
					const float* yvals,int sz);
				//!< Undef values are filtered out
    virtual void		setY2Vals(const float* xvals,
					  const float* yvals,int sz);
    virtual void		setY2Vals(const Interval<float>&,
					  const float* yvals,int sz);
				//!< Undef values are filtered out
    virtual void		setMarkValue(float,bool is_x);
    virtual void		setMark2Value(float,bool is_x);
    virtual void		setEmpty(); //!< clears all

    const TypeSet<float>&	xVals() const	{ return xvals_; }
    const TypeSet<float>&	yVals() const	{ return yvals_; }
    const TypeSet<float>&	y2xVals() const { return y2xvals_; }
    const TypeSet<float>&	y2yVals() const { return y2yvals_; }
    int				size() const	{ return xvals_.size(); }
    int				y2size() const	{ return y2xvals_.size(); }

    void			getAxisRanges(const TypeSet<float>& vals,
					      const Interval<float>& setuprg,
					      Interval<float>&) const;

    uiAxisHandlerBase*		xAxis()		{ return  xax_; }
    uiAxisHandlerBase*		yAxis( bool y2) { return  y2 ? y2ax_ : yax_; }
    const uiAxisHandlerBase*	xAxis() const	{ return  xax_; }
    const uiAxisHandlerBase*	yAxis( bool y2) const
						{ return  y2 ? y2ax_ : yax_; }

    virtual Geom::PointF	mapToPosition(const Geom::PointF&,
					      bool y2=false) = 0;
    virtual Geom::PointF	mapToValue(const Geom::PointF&,
					   bool y2=false) = 0;

    void			dump(od_ostream&,bool y2) const;
    int				selPt() const	{ return selpt_; }

    virtual void		setTitle(const uiString&) = 0;
    virtual void		gatherInfo(bool y2=false);
    virtual void		draw() = 0;
    virtual uiObject*		uiobj() = 0;
    virtual const NotifierAccess&	mouseMove() = 0;
    virtual Geom::PointF		mousePos() = 0;

protected:

    Setup			setup_;

    uiAxisHandlerBase*		xax_ = nullptr;
    uiAxisHandlerBase*		yax_ = nullptr;
    uiAxisHandlerBase*		y2ax_ = nullptr;

    float			xmarklineval_;
    float			ymarklineval_;
    float			xmarkline2val_;
    float			ymarkline2val_;

    int				selpt_;

    TypeSet<float>		xvals_;
    TypeSet<float>		yvals_;
    TypeSet<float>		y2xvals_;
    TypeSet<float>		y2yvals_;

    virtual void		cleanUp() = 0;
    virtual void		drawMarkLines() = 0;


};


