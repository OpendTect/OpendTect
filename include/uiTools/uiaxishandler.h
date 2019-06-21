#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "draw.h"
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
\brief Handles an axis on a plot.

  Manages the positions in a 2D plot. The axis can be logarithmic. getRelPos
  returns the relative position on the axis. If the point is between the
  start and stop of the range, this will be between 0 and 1.

  The axis will determine a good position wrt the border. To determine where
  the axis starts and stops, you can provide other axis handlers. If you don't
  provide these, the border_ will be used. The border_ on the side of the axis
  will always be used. If you do use begin and end handlers, you'll have to
  call setRange() for all before using plotAxis().

  The drawAxis will plot the axis. If OD::LineStyle::Type is not
  OD::LineStyle::None,
  grid lines will be drawn, too. If it *is* None, then still the color and
  size will be used for drawing the axis (the axis' style is always Solid).

  Use AxisLayout (linear.h) to find 'nice' ranges, like:
  AxisLayout al( Interval<float>(start,stop) );
  ahndlr.setRange( StepInterval<float>(al.sd.start,al.stop,al.sd.step) );
*/

mExpClass(uiTools) uiAxisHandler
{
public:

    struct Setup
    {
			Setup( uiRect::Side s, int w=0, int h=0 )
			    : side_(s)
			    , noaxisline_(false)
			    , noaxisannot_(false)
			    , nogridline_(false)
			    , showauxannot_(false)
			    , auxlinestyle_(OD::LineStyle(OD::LineStyle::Dot))
			    , annotinside_(false)
			    , annotinint_(false)
			    , fixedborder_(false)
			    , ticsz_(2)
			    , width_(w)
			    , height_(h)
			    , maxnrchars_(0)
			    , specialvalue_(0.0f)
			    , islog_(false)
			    , zval_(4)
			    , nmcolor_(Color::NoColor())
			    , fontdata_(FontData(10))
			    {}

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
	mDefSetupMemb(Color,nmcolor)
	mDefSetupMemb(FontData,fontdata)

	Setup&		noannot( bool yn )
			{ noaxisline_ = noaxisannot_ = nogridline_ = yn;
			  return *this; }

	void		setShowSpecialValue( bool yn, float newval=0.0f )
			{ specialvalue_ = yn ? newval : mUdf(float); }
	bool		showSpecialValue() const
			{ return !mIsUdf(specialvalue_); }
    };

			uiAxisHandler(uiGraphicsScene*,const Setup&);
			~uiAxisHandler();

    void		setCaption(const uiString&);
    uiString		getCaption() const	{ return setup_.caption_; }
    void		setBorder(const uiBorder&);
    void		setIsLog(bool yn);
    void		setBegin( const uiAxisHandler* ah )
						{ beghndlr_ = ah; newDevSize();}
    void		setEnd( const uiAxisHandler* ah )
						{ endhndlr_ = ah; newDevSize();}

    void		setRange(const StepInterval<float>&,float* axstart=0);
    void		setBounds(Interval<float>); //!< makes annot 'nice'

    float		getVal(int pix) const;
    float		getRelPos(float absval) const;
    int			getPix(float absval) const;
    int			getPix(double abvsval) const;
    int			getPix(int) const;
    int			getRelPosPix(float relpos) const;
    void		setAuxAnnot( const TypeSet<OD::PlotAnnotation>& pos )
						{ auxannots_ = pos; }

    const Setup&	setup() const		{ return setup_; }
    Setup&		setup()			{ return setup_; }
    StepInterval<float> range() const		{ return datarg_; }
    float		annotStart() const	{ return annotstart_; }
    bool		isHor() const	{ return uiRect::isHor(setup_.side_); }
    int			pixToEdge(bool withborder=true) const;
    int			pixBefore() const;
    int			pixAfter() const;
    Interval<int>	pixRange() const;

			//!< Call this when appropriate
    void		newDevSize();
    void		updateDevSize();	//!< resized from sceme
    void		setNewDevSize(int,int); //!< resized by yourself
    void		updateScene();

    uiLineItem*		getTickLine(int pix);
    uiLineItem*		getGridLine(int pix);
    int			getNrAnnotCharsForDisp() const;
    void		setVisible(bool);
    void		annotAtEnd(const uiString&);

protected:

    uiGraphicsScene*	scene_;
    uiTextItem*		nameitm_;
    uiTextItem*		endannotitm_;
    uiLineItem*		axislineitm_;

    Setup		setup_;
    bool		islog_;
    StepInterval<float> datarg_;
    float		annotstart_;
    uiBorder		border_;
    int			ticsz_;
    int			height_;
    int			width_;
    int			reqnrchars_;
    const uiAxisHandler* beghndlr_;
    const uiAxisHandler* endhndlr_;

    int			pxsizeinotherdir_;
    uiAHPlotAnnotSet&	annots_;
    TypeSet<OD::PlotAnnotation> auxannots_;
    float		endpos_;
    int			devsz_;
    int			axsz_;
    bool		rgisrev_;
    bool		ynmtxtvertical_;
    float		rgwidth_;
    float		epsilon_;
    StepInterval<float> annotrg_;
    int			nrsteps_;

    int			ticSz() const
				{ return setup_.noaxisannot_ ? 0 : ticsz_; }
    int			tickEndPix(bool farend) const;
    void		updateAxisLine();
    bool		reCalcAnnotation();
    void		updateName();

    friend class	uiAHPlotAnnotSet;

};

//! draws line not outside box defined by X and Y value ranges
mGlobal(uiTools) void setLine(uiLineItem&,const LineParameters<float>&,
			const uiAxisHandler& xah,const uiAxisHandler& yah,
			const Interval<float>* xvalrg = 0);
