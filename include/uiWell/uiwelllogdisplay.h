#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.4 2009-06-15 09:53:03 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "draw.h"

class uiTextItem;
class uiPolyLineItem;
class uiGraphicsScene;
class UnitOfMeasure;
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
	void			copySetupFrom( const LogData& ld )
	    			{ unitmeas_ = ld.unitmeas_; xrev_ = ld.xrev_;
				  linestyle_ = ld.linestyle_;
				  logarithmic_ = ld.logarithmic_;
				  clipratio_ = ld.clipratio_; }

	// Set these
	const Well::Log*	wl_;
	const UnitOfMeasure*	unitmeas_;
	bool			xrev_;
	LineStyle		linestyle_;
	bool			logarithmic_;
	float			clipratio_;

	// Get these (will be filled)
	Interval<float>		zrg_;
	Interval<float>		valrg_;
	uiAxisHandler		xax_;
	uiAxisHandler		yax_;
	ObjectSet<uiPolyLineItem> curveitms_;
	uiTextItem*		curvenmitm_;
    };

    LogData&			logData( bool first=true )
				{ return first ? ld1_ : ld2_; }

    const Interval<float>&	zRange() const	{ return zrg_; }
    void			setZRange(const Interval<float>&);
    void			dataChanged();

    void			setZDispInFeet( bool yn )
    				{ dispzinft_ = yn; dataChanged(); }
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

    void			setAxisRelations();
    void			gatherInfo(bool);
    void			setAxisRanges(bool);
    void			drawCurve(bool);

};


#endif
