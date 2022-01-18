#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uiaxishandlerbase.h"
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
\brief Handles an axis on a plot.

  Manages the positions in a 2D plot. The axis can be logarithmic. getRelPos
  returns the relative position on the axis. If the point is between the
  start and stop of the range, this will be between 0 and 1.

  The axis will determine a good position wrt the border. To determine where
  the axis starts and stops, you can provide other axis handlers. If you don't
  provide these, the border_ will be used. The border_ on the side of the axis
  will always be used. If you do use begin and end handlers, you'll have to
  call setRange() for all before using plotAxis().

  The drawAxis will plot the axis. If LineStyle::Type is not LineStyle::None,
  grid lines will be drawn, too. If it *is* None, then still the color and
  size will be used for drawing the axis (the axis' style is always Solid).

  Use AxisLayout (linear.h) to find 'nice' ranges, like:
  AxisLayout al( Interval<float>(start,stop) );
  ahndlr.setRange( StepInterval<float>(al.sd.start,al.stop,al.sd.step) );
*/

mExpClass(uiTools) uiAxisHandler : public uiAxisHandlerBase
{
public:
			uiAxisHandler(uiGraphicsScene*,const Setup&);
			~uiAxisHandler();

    void		setCaption(const uiString&) override;
    uiString		getCaption() const	{ return setup_.caption_; }
    void		setBorder(const uiBorder&) override;
    void		setIsLog(bool) override;
    void		setBegin(const uiAxisHandler*);
    void		setEnd(const uiAxisHandler*);

    void		setRange(const StepInterval<float>&,
				 float* axst=nullptr) override;
    void		setBounds(Interval<float>) override;

    float		getVal(int pix) const;
    float		getRelPos(float absval) const;
    int			getPix(float absval) const;
    int			getPix(double abvsval) const;
    int			getPix(int) const;
    int			getRelPosPix(float relpos) const;
    void		setAuxAnnot(const TypeSet<PlotAnnotation>& pos)
						{ auxannots_ = pos; }

    StepInterval<float> range() const		{ return datarg_; }
    float		annotStart() const	{ return annotstart_; }
    int			pixToEdge(bool withborder=true) const;
    int			pixBefore() const;
    int			pixAfter() const;
    Interval<int>	pixRange() const;

			//!< Call this when appropriate
    void		newDevSize();
    void		updateDevSize();	//!< resized from sceme
    void		setNewDevSize(int,int); //!< resized by yourself
    void		updateScene();

    uiLineItem*		getTickLine(int);
    uiLineItem*		getGridLine(int);
    int			getNrAnnotCharsForDisp() const;
    void		setVisible(bool);
    void		annotAtEnd(const uiString&);

protected:

    uiGraphicsScene*	scene_;
    uiTextItem*		nameitm_;
    uiTextItem*		endannotitm_;
    uiLineItem*		axislineitm_;

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
    TypeSet<PlotAnnotation> auxannots_;
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
    int			tickEndPix(bool) const;
    void		updateAxisLine();
    bool		reCalcAnnotation();
    void		updateName();

    friend class	uiAHPlotAnnotSet;

};

//! draws line not outside box defined by X and Y value ranges
mGlobal(uiTools) void setLine(uiLineItem*,const LineParameters<float>&,
			const uiAxisHandler*,const uiAxisHandler*,
			const Interval<float>* xvalrg=0);


