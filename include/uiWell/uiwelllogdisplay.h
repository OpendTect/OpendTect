#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.8 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "draw.h"

class uiTextItem;
class uiLineItem;
class uiPolyLineItem;
class uiGraphicsScene;
class UnitOfMeasure;
namespace Well { class Log; class Marker; }

/*!\brief creates a display of max 2 well logs. */

mClass uiWellLogDisplay : public uiGraphicsView
{
public:

    mStruct Setup
    {
				Setup()
				    : nrmarkerchars_(2)
				    , markerls_(LineStyle::Dot,1)
				    , pickls_(LineStyle::Solid,1,Color(0,200,0))
				    , border_(5)		{}

	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(int,nrmarkerchars)  //!< Will display up to this nr chars
	mDefSetupMemb(LineStyle,markerls) //!< will not use color
	mDefSetupMemb(LineStyle,pickls)	  //!< color used if no PickData color
    };

				uiWellLogDisplay(uiParent*,const Setup&);
				~uiWellLogDisplay();

    mStruct LogData
    {
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

    protected:

				LogData(uiGraphicsScene&,bool isfirst,
					const uiBorder&);
	void			copySetupFrom( const LogData& ld )
	    			{ unitmeas_ = ld.unitmeas_; xrev_ = ld.xrev_;
				  linestyle_ = ld.linestyle_;
				  logarithmic_ = ld.logarithmic_;
				  clipratio_ = ld.clipratio_; }
	friend class		uiWellLogDisplay;
    };

    LogData&			logData( bool first=true )
				{ return first ? ld1_ : ld2_; }
    void			setMarkers( const ObjectSet<Well::Marker>* ms )
				{ markers_ = ms; }

    mStruct PickData
    {
			PickData( float dah, Color c=Color::NoColor() )
			    : dah_(dah), color_(c)	{}
	bool		operator ==( const PickData& pd ) const
	    		{ return mIsEqual(pd.dah_,dah_,1e-4); }

	float		dah_;
	Color		color_;	//!< default will use the global setup color
    };
    TypeSet<PickData>&		zPicks()	{ return zpicks_; }

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
    const ObjectSet<Well::Marker>* markers_;
    TypeSet<PickData>		zpicks_;
    Setup			setup_;

    ObjectSet<uiLineItem>	markeritms_;
    ObjectSet<uiTextItem>	markertxtitms_;
    ObjectSet<uiLineItem>	zpickitms_;

    void			init(CallBacker*);
    void			mouseRelease(CallBacker*);
    void			reSized(CallBacker*);

    void			gatherInfo();
    void			draw();

    void			setAxisRelations();
    void			gatherInfo(bool);
    void			setAxisRanges(bool);
    void			drawCurve(bool);
    void			drawMarkers();
    void			drawZPicks();

};


#endif
