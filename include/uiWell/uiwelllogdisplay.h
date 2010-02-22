#ifndef uidirectionalplot_h
#define uidirectionalplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.20 2010-02-22 09:34:19 cvsbruno Exp $
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
class uiParent;
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

#define mLogWidth 170
#define mLogHeight 600

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
				    , nogridline_(false)
				    , border_(5)
				    , yborder_(5)
				    , noborderspace_(false)
				    , axisticsz_(2)			   
				    , noxpixbefore_(false)
				    , noypixbefore_(false)
				    , noxpixafter_(false)
				    , noypixafter_(false)
				    , noxaxisline_(false)
				    , noyaxisline_(false)
				    {}

	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(uiBorder,yborder)
	mDefSetupMemb(int,nrmarkerchars)  //!< Will display up to this nr chars
	mDefSetupMemb(LineStyle,markerls) //!< will not use color
	mDefSetupMemb(LineStyle,pickls)	  //!< color used if no PickData color
	mDefSetupMemb(bool,noxpixbefore)
	mDefSetupMemb(bool,noypixbefore)
	mDefSetupMemb(bool,noxpixafter)
	mDefSetupMemb(bool,noypixafter)
	mDefSetupMemb(bool,noborderspace)
	mDefSetupMemb(int,axisticsz)
	mDefSetupMemb(bool,nogridline)
	mDefSetupMemb(bool,noyaxisline)
	mDefSetupMemb(bool,noxaxisline)
    };

				uiWellLogDisplay(uiParent*,const Setup&);
				~uiWellLogDisplay();

    mStruct LineData
    {
	    mStruct Setup
	    {
				Setup()
				    : xborder_(5)
				    , yborder_(5)
				    , noborderspace_(false)
				    , xaxisticsz_(2)			   
				    , nogridline_(false)
				    , noxaxisline_(false)
				    , noyaxisline_(false)
				    {}

		mDefSetupMemb(uiBorder,xborder)
		mDefSetupMemb(uiBorder,yborder)
		mDefSetupMemb(bool,noborderspace)
		mDefSetupMemb(int,xaxisticsz)
		mDefSetupMemb(bool,noyaxisline)
		mDefSetupMemb(bool,noxaxisline)
		mDefSetupMemb(bool,nogridline)
	    };

				LineData(uiGraphicsScene&,
					const Setup&);

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
				LogData(uiGraphicsScene&,int idx,const Setup&);

	void			copySetupFrom( const LogData& ld )
	    			{ unitmeas_ = ld.unitmeas_; xrev_ = ld.xrev_; }
	friend class		uiWellLogDisplay;
    };
    
    LogData&			logData(int);
    LogData&			addLogData();

    int logNr() const		{ return lds_.size(); }

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
    
protected:

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


mClass uiWellDisplay : public uiGroup
{
public:

    mStruct Setup
    {
				Setup()
				    : left_(true)
				    , right_(true)
				    , noborderspace_(false)
				    , logwidth_(mLogWidth)	 	   
				    , logheight_(mLogHeight)	 	   
				    {}

	mDefSetupMemb(bool,left) // Left Log
	mDefSetupMemb(bool,right) // Right log
	mDefSetupMemb(bool,noborderspace) // Right log
	mDefSetupMemb(int,logheight) //log height
	mDefSetupMemb(int,logwidth) //log width
    };

				uiWellDisplay(uiParent*,const Setup&,
					      const Well::Data&);
				~uiWellDisplay(){};
    
    uiWellLogDisplay* 		leftDisplay() { return leftlogdisp_; }
    uiWellLogDisplay* 		rightDisplay() { return rightlogdisp_; }

    void			addLog(const char*,bool isleft);
    void			removeLog(const char*,bool isleft);
    void			removeLogPanel(bool isleft);
    
    void			setInitialZRange();
    const Interval<float>&	zRange() const	{ return zrg_; }
    void			setZRange(const Interval<float>&);
    void			setZInTime( bool yn )
    				{ zintime_ = yn; dataChanged(0); }
    bool			zInTime() const	  { return zintime_; }
    int 			displayWidth() const;
    void 			updateProperties(CallBacker*); 

protected:

    const Well::Data&		wd_;
    const Well::D2TModel*	d2tm_;
    uiParent*			parent_;

    Interval<float>		zrg_;
    bool			zintime_;
    bool			dispzinft_;
    int 			logwidth_;	 	   
    int 			logheight_;
    
    uiWellLogDisplay* 		leftlogdisp_;
    uiWellLogDisplay* 		rightlogdisp_;
    void			addLogPanel(bool,bool);
    void                        gatherInfo();
    void                        setAxisRanges();
    void                        dataChanged(CallBacker*);
};


mClass uiWellDisplayWin : public uiMainWin
{
public:
    			uiWellDisplayWin(uiParent* p, const Well::Data& wd )
			    : uiMainWin(p,"") 
			    , wellview_(*new uiWellDisplay(this,
					    uiWellDisplay::Setup(),wd))
			{
			    BufferString msg( "2D Viewer ");
			    msg += wd.name();
			    setCaption( msg );
			}

protected:

    uiWellDisplay& 	wellview_;
};

#endif
