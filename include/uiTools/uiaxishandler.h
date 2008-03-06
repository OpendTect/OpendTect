#ifndef uiaxishandler_h
#define uiaxishandler_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uiaxishandler.h,v 1.1 2008-03-06 14:16:58 cvsbert Exp $
________________________________________________________________________

-*/

#include "draw.h"
#include "ranges.h"
#include "bufstringset.h"
#include "uigeom.h"
class ioDrawTool;

/*!\brief Handles an axis on a plot

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
  size will be used for drawing the axis (which  is laways Solid).

  Use AxisLayout (linear.h) to find 'nice' ranges, like:
  AxisLayout al( start, stop );
  ahndlr.setRange( StepInterval<float>(al.start,al.sd.step,al.sd.step) );
 
 */

class uiAxisHandler
{
public:

			uiAxisHandler(ioDrawTool&,uiRect::Side);
    void		setRange( const StepInterval<float>& rg )
						{ rg_ = rg; reCalc(); }
    void		setIsLog( bool yn )	{ islog_ = yn; reCalc(); }
    void		setBorder( const uiBorder& b )
						{ border_ = b; }
    void		setBegin( const uiAxisHandler* ah )
						{ beghndlr_ = ah; }
    void		setEnd( const uiAxisHandler* ah )
						{ endhndlr_ = ah; }

    int			pixToEdge() const;
    int			pixBefore() const;
    int			pixAfter() const;
    float		getRelPos(float) const;
    int			getPix(float) const;

    void		plotAxis(LineStyle) const;

    uiRect::Side	side() const	{ return side_; }
    StepInterval<float>	range() const	{ return rg_; }
    uiBorder		border() const	{ return border_; }
    bool		isLog() const	{ return islog_; }

    bool		isHor() const	{ return uiRect::isHor(side_); }

protected:

    ioDrawTool&		dt_;
    const uiRect::Side	side_;
    bool		islog_;
    StepInterval<float>	rg_;
    uiBorder		border_;
    int			ticsz_;
    const uiAxisHandler* beghndlr_;
    const uiAxisHandler* endhndlr_;

    void		reCalc();
    int			wdthx_;
    int			wdthy_;
    BufferStringSet	strs_;
    TypeSet<float>	pos_;
    int			devsz_;
    int			axsz_;

    int			getRelPosPix(float) const;
    void		drawAxisLine(LineStyle) const;
    void		annotPos(int,const char*) const;
    void		drawGridLine(int) const;

};


#endif
