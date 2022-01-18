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
#include "bufstringset.h"
#include "namedobj.h"
#include "uigeom.h"
#include "uistring.h"
#include "fontdata.h"

class uiGraphicsScene;
class uiLineItem;
class uiTextItem;
class uiAHPlotAnnotSet;
template <class T> class LineParameters;

/*!
\brief Base class for Axis Handlers

*/

mExpClass(uiTools) uiAxisHandlerBase
{
public:

    struct Setup
    {
	Setup( uiRect::Side s, int w=0, int h=0 )
	    : side_(s)
	    , width_(w)
	    , height_(h)
	    , islog_(false)
	    , noaxisline_(false)
	    , noaxisannot_(false)
	    , nogridline_(false)
	    , annotinside_(false)
	    , annotinint_(false)
	    , fixedborder_(false)
	    , showauxannot_(false)
	    , ticsz_(2)
	    , maxnrchars_(0)
	    , specialvalue_(0.0f)
	    , zval_(0)
	    , nmcolor_(OD::Color::NoColor())
	    , fontdata_(FontData(10))
	{
	    gridlinestyle_.color_ = OD::Color(200,200,200);
	    auxlinestyle_.type_ = OD::LineStyle::Dot;
	}

	mDefSetupMemb(uiRect::Side,side)
	mDefSetupMemb(int,width)
	mDefSetupMemb(int,height)
	mDefSetupMemb(bool,islog)
	mDefSetupMemb(bool,noaxisline)
	mDefSetupMemb(bool,noaxisannot)
	mDefSetupMemb(bool,nogridline)
	mDefSetupMemb(bool,annotinside)
	mDefSetupMemb(bool,annotinint)
	mDefSetupMemb(bool,fixedborder)
	mDefSetupMemb(bool,showauxannot)
	mDefSetupMemb(int,ticsz)
	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(OD::LineStyle,style)
	mDefSetupMemb(OD::LineStyle,gridlinestyle)
	mDefSetupMemb(OD::LineStyle,auxlinestyle)
	mDefSetupMemb(OD::LineStyle,auxhllinestyle)
	mDefSetupMemb(uiString,caption)
	mDefSetupMemb(int,maxnrchars)
	mDefSetupMemb(float,specialvalue) //!< Will be gridlined and annotated.
	mDefSetupMemb(int,zval)
	mDefSetupMemb(OD::Color,nmcolor)
	mDefSetupMemb(FontData,fontdata)

	Setup&		noannot( bool yn )
			{ noaxisline_ = noaxisannot_ = nogridline_ = yn;
			  return *this; }

	void		setShowSpecialValue( bool yn, float newval=0.0f )
			{ specialvalue_ = yn ? newval : mUdf(float); }
	bool		showSpecialValue() const
			{ return !mIsUdf(specialvalue_); }
    };

    virtual		~uiAxisHandlerBase() {}

    const Setup&	setup() const	{ return setup_; }
    Setup&		setup()		{ return setup_; }
    bool		isHor() const	{ return uiRect::isHor(setup_.side_); };

    virtual void	setCaption(const uiString&) = 0;
    virtual uiString	getCaption() const = 0;
    virtual void	setBorder(const uiBorder&) = 0;
    virtual void	setIsLog(bool) = 0;

    virtual void	setRange(const StepInterval<float>&,
				 float* axst=nullptr) = 0;
    virtual void	setBounds(Interval<float>) = 0;

    virtual StepInterval<float> range() const = 0;

protected:
			uiAxisHandlerBase(const Setup& su)
			    : setup_(su)			{}

    Setup		setup_;
};
