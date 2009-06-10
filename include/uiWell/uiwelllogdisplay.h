#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.1 2009-06-10 13:21:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "draw.h"

class uiTextItem;
class uiPolyLineItem;
class uiGraphicsScene;
namespace Well { class Log; }

/*!\brief creates a display of max 2 well logs. */

mClass uiWellLogDisplay : public uiGraphicsView
{
public:

				uiWellLogDisplay(uiParent*);
				~uiWellLogDisplay();

    mStruct LogData
    {
				LogData(uiGraphicsScene&,bool isfirst);

	// Set these
	const Well::Log*	wl_;
	Color			color_;
	LineStyle		linestyle_;
	bool			logarithmic_;
	float			clipratio_;

	// Get these (will be filled)
	Interval<float>		zrg_;
	Interval<float>		valrg_;
	uiAxisHandler		xax_;
	uiAxisHandler		yax_;
	uiPolyLineItem*		curveitm_;
	uiTextItem*		curvenmitm_;

	void			setSecond(LogData&);
    };

    LogData&			logData( bool first=true )
				{ return first ? ld1_ : ld2_; }

    const Interval<float>&	zRange() const	{ return zrg_; }
    void			setZRange(const Interval<float>&);
    void			dataChanged();

    void			setZDispInFeet( bool yn ) { dispzinft_ = yn; }
    bool			zDispInFeet() const	  { return dispzinft_; }

protected:

    LogData			ld1_;
    LogData			ld2_;
    Interval<float>		datazrg_;
    Interval<float>		zrg_;
    bool			dispzinft_;

    void			init(CallBacker*);
    void			mouseRelease(CallBacker*);
    void			reSized(CallBacker*);

    void			gatherInfo();
    void			draw();

    void			gatherInfo(bool);
    void			drawCurve(bool);

};


#endif
