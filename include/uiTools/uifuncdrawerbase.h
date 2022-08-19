#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "color.h"
#include "mathfunc.h"

class uiAxisHandlerBase;
class CallBacker;

/*!brief Displays a mathfunction. */

mExpClass(uiTools) uiFuncDrawerBase
{
public:
    mStruct(uiTools) Setup
    {
			Setup()
			    : xaxrg_(-1.2,1.2,0.25)
			    , yaxrg_(0,1,0.25)
			    , funcrg_(-1.2,1.2)
			    , canvaswidth_(400)
			    , canvasheight_(250)
			    , width_(2)
			    , name_(nullptr)
			{}

	mDefSetupMemb(StepInterval<float>,xaxrg)
	mDefSetupMemb(StepInterval<float>,yaxrg)
	mDefSetupMemb(int,canvaswidth)
	mDefSetupMemb(int,canvasheight)
	mDefSetupMemb(int,width)
	mDefSetupMemb(const char*,name)
	mDefSetupMemb(uiString,xaxcaption)
	mDefSetupMemb(uiString,yaxcaption)
	mDefSetupMemb(Interval<float>,funcrg)
    };

    mStruct(uiTools) DrawFunction
    {
		DrawFunction( const FloatMathFunction* f )
		    : color_(OD::Color::DgbColor())
		    , mathfunc_(f)
		    {}

	const FloatMathFunction*    mathfunc_;
	TypeSet<uiPoint>	    pointlist_;
	OD::Color		    color_;
    };

			uiFuncDrawerBase(const Setup&);
			~uiFuncDrawerBase();

    Setup&			setup()		{ return setup_; }
    uiAxisHandlerBase*		xAxis()		{ return  xax_; }
    const uiAxisHandlerBase*	xAxis() const	{ return  xax_; }
    uiAxisHandlerBase*		yAxis()		{ return  yax_; }
    const uiAxisHandlerBase*	yAxis() const	{ return  yax_; }

    void		addFunction(DrawFunction* f) { functions_ += f; }
    void		clearFunctions(){ deepErase( functions_ ); }
    void		clearFunction(int);
    Interval<float>&	getFunctionRange() { return funcrg_; }
    void		setSelItems(TypeSet<int> s) { selitemsidx_ = s; }
    void		setFunctionRange(Interval<float>& rg) {funcrg_ = rg;}

    virtual void	draw(CallBacker*) = 0;
    virtual uiObject*	uiobj() = 0;

protected:
    Setup		setup_;
    Interval<float>	funcrg_;
    float		variable_;
    uiAxisHandlerBase*	xax_;
    uiAxisHandlerBase*	yax_;
    ObjectSet<DrawFunction> functions_;
    TypeSet<int>	selitemsidx_;
};
