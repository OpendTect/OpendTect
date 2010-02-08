#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.16 2010-02-08 16:43:44 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uimainwin.h"
#include "uigroup.h"
#include "uiaxishandler.h"
#include "draw.h"
#include "welldata.h"
#include "wellmarker.h"
#include "welldisp.h"

class uiGraphicsScene;
class uiLineItem;
class uiObjectItem;
class uiPolyLineItem;
class uiPolygonItem;
class uiTextItem;
class uiToolBar;
class uiWellDispPropDlg;
class UnitOfMeasure;
namespace Well 
{ 
    class Log; 
    class D2TModel; 
    class Track; 
    class DahObj;
}

#define mPanelWidth 200
#define mPanelHeight 600

/*!\brief creates a display of max 2 well logs. */

mClass uiWellLogDisplay : public uiGroup
{
public:

    mStruct Setup
    {
				Setup()
				    : nrmarkerchars_(2)
				    , markerls_(LineStyle::Dot,1)
				    , pickls_(LineStyle::Solid,1,Color(0,200,0))
				    , border_(5)
				    , noxpixbefore_(false)
				    , noypixbefore_(false)
				    , noxpixafter_(false)
				    , noypixafter_(false)
				    {}

	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(int,nrmarkerchars)  //!< Will display up to this nr chars
	mDefSetupMemb(LineStyle,markerls) //!< will not use color
	mDefSetupMemb(LineStyle,pickls)	  //!< color used if no PickData color
	mDefSetupMemb(bool,noxpixbefore)
	mDefSetupMemb(bool,noypixbefore)
	mDefSetupMemb(bool,noxpixafter)
	mDefSetupMemb(bool,noypixafter)
    };

				uiWellLogDisplay(uiParent*,const Setup&);
				~uiWellLogDisplay();

    mStruct LineData
    {
				LineData(uiGraphicsScene&,const uiBorder&);

	Interval<float>		zrg_;
	Interval<float>		valrg_;
	uiAxisHandler		xax_;
	uiAxisHandler		yax_;
	
	// Get these (will be filled)
	ObjectSet<uiPolyLineItem> curveitms_;
	ObjectSet<uiPolygonItem> curvepolyitms_;
	uiTextItem*		curvenmitm_;
	Alignment 		al_;
    };

    mStruct LogData : public LineData
    {
	// Set these
	const Well::Log*	wl_;
	const UnitOfMeasure*	unitmeas_;
	bool			xrev_;
	bool			isyaxisleft_;

	Well::DisplayProperties::Log wld_;

	protected:
				LogData(uiGraphicsScene& s,int idx,
					const uiBorder& b);

	void			copySetupFrom( const LogData& ld )
	    			{ unitmeas_ = ld.unitmeas_; xrev_ = ld.xrev_; }
	friend class		uiWellLogDisplay;
    };
    
    LogData&			logData(int);
    LogData&			addLogData();

    void                        setD2TModel( const Well::D2TModel* d2tm )
				{ d2tm_ = d2tm; }
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

    void			setZInTime( bool yn )
    				{ zintime_ = yn; dataChanged(); }
    bool			zInTime() const	  { return zintime_; }
    
    uiGraphicsScene&		scene() { return viewer_->scene(); }
    uiGraphicsView*		viewer() { return viewer_; }

protected:

    uiGraphicsView*		viewer_;
	
    ObjectSet<LogData>		lds_;
    Interval<float>		zrg_;
    bool			zintime_;
    bool			dispzinft_;

    const ObjectSet<Well::Marker>* markers_;
    TypeSet<PickData>		zpicks_;
    const Well::D2TModel*       d2tm_;

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
    void			gatherInfo(int);
    void			setAxisRanges(int);
    void			drawLog(int);
    void			drawLine(LogData&,const Well::DahObj* ldah);
    void			drawFilling(int);
    void			drawMarkers();
    void			drawZPicks();
    
    void			removeLog(const char*);
    void			removeAllLogs();

    friend class 		uiWellDisplay;
};


mClass uiWellDisplay : public uiGraphicsView
{
public:

    mStruct Setup
    {
				Setup()
				    : left_(true)
				    , right_(true)
				    {}

	mDefSetupMemb(bool,left) // Left Log
	mDefSetupMemb(bool,right) // Right log
    };

				uiWellDisplay(uiParent*,const Setup&,
					      const Well::Data&);
				~uiWellDisplay();
    
    void			addLog(const char*,bool isleft);
    void			removeLog(const char*,bool isleft);
    void			removeLogPanel(bool isleft);
    
    mStruct TrackData : public uiWellLogDisplay::LineData
    {
	const Well::Track*	wt_;
	Well::DisplayProperties::Track wtd_;
	
	protected:
				TrackData(uiGraphicsScene& sc,const uiBorder& b)
				    : uiWellLogDisplay::LineData(sc,b)
				    , wt_(0)  
				    {}

	friend class 		uiWellDisplay;
    };

    void			drawTrack();
    TrackData			td_;
    
    const Interval<float>&	zRange() const	{ return zrg_; }
    
    void 			updateProperties( 
	    				const Well::DisplayProperties& );

protected:

    const Well::Data&		wd_;
    const Well::D2TModel*	d2tm_;

    Interval<float>		zrg_;
    bool			zintime_;
    bool			dispzinft_;
    
    uiWellLogDisplay* 		leftlogdisp_;
    uiWellLogDisplay* 		rightlogdisp_;

    uiObjectItem* 		leftlogitm_;
    uiObjectItem* 		rightlogitm_;
    

    void			addLogPanel(bool);
    void                        gatherInfo();
    void                        setAxisRanges();
    void                        dataChanged(CallBacker*);
};


mClass uiWellDisplayWin : public uiMainWin
{
public:

				uiWellDisplayWin(uiParent*,Well::Data&);
				~uiWellDisplayWin();

protected:

    Well::Data& 		wd_;
    uiToolBar* 			toolbar_;
    uiWellDisplay* 		logviewer_;
    uiWellDispPropDlg* 		propdlg_;

    void			propDlgPushed(CallBacker*);
    void 			updateProperties(CallBacker*);
    friend class 		uiWellDisplay;
};


#endif
