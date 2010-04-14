#ifndef uiwelllogdisplay_h
#define uiwelllogdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.31 2010-04-14 15:36:16 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uimainwin.h"
#include "uigroup.h"
#include "uiaxishandler.h"
#include "uicursor.h"
#include "draw.h"
#include "welldisp.h"

class uiGraphicsScene;
class uiLineItem;
class uiMenuHandler;
class uiPolyLineItem;
class uiPolygonItem;
class uiTextItem;
class uiWellStratDisplay;
class uiWellDisplayMarkerEdit;

class Params;
class UnitOfMeasure;
namespace Well 
{ 
    class Log; 
    class D2TModel; 
    class Data;
    class Marker;
    class MarkerSet;
    class Track; 
    class DahObj;
}

#define mStratWidth 75
#define mLogWidth 150
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
				    , noxgridline_(false)
				    , noygridline_(false)
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
				    , nobackground_(false)		 
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
	mDefSetupMemb(bool,noygridline)
	mDefSetupMemb(bool,noxgridline)
	mDefSetupMemb(bool,noyaxisline)
	mDefSetupMemb(bool,noxaxisline)
	mDefSetupMemb(bool,nobackground)
    };

				uiWellLogDisplay(uiParent*,const Setup&);
				~uiWellLogDisplay(){};

    mStruct LineData
    {
	    mStruct Setup
	    {
				Setup()
				    : border_(5)
				    , noborderspace_(false)
				    , xaxisticsz_(2)			   
				    , noxgridline_(false)
				    , noygridline_(false)
				    , noxaxisline_(false)
				    , noyaxisline_(false)
				    {}

		mDefSetupMemb(uiBorder,border)
		mDefSetupMemb(bool,noborderspace)
		mDefSetupMemb(int,xaxisticsz)
		mDefSetupMemb(bool,noyaxisline)
		mDefSetupMemb(bool,noxaxisline)
		mDefSetupMemb(bool,noxgridline)
		mDefSetupMemb(bool,noygridline)
	    };

				LineData(uiGraphicsScene&,Setup);

	Interval<float>		zrg_;
	Interval<float>		valrg_;
	uiAxisHandler		xax_;
	uiAxisHandler		yax_;
	Setup 			setup_;
	
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
				LogData(uiGraphicsScene&,bool,Setup);

	void			copySetupFrom( const LogData& ld )
	    			{ unitmeas_ = ld.unitmeas_; xrev_ = ld.xrev_; }
	friend class		uiWellLogDisplay;
    };
    
    LogData&			logData(bool first=true)
    				{ return first ? ld1_ : ld2_; }
    void			dataChanged();

    mStruct MarkerItem
    {
			MarkerItem(Well::Marker&);
			~MarkerItem();

	Well::Marker&	mrk_;
	uiLineItem*	itm_;
	uiTextItem*	txtitm_;
	Color		color_;	
    };

    Notifier<uiWellLogDisplay>  highlightedMarkerItemChged;
    void 			highlightMarkerItem(const Well::Marker*);
    MarkerItem* 		getMarkerItem(const Well::Marker*);
    const Well::Marker*		highlightedmrk_;
    ObjectSet<MarkerItem>&   	markerItems() { return  markeritms_; } 
    void			reDrawMarkers(CallBacker*);

    mStruct PickData
    {
			PickData( float dah, Color c=Color::NoColor() )
			    : dah_(dah), color_(c)	{}
	bool		operator ==( const PickData& pd ) const
	    		{ return mIsEqual(pd.dah_,dah_,1e-4); }

	float		dah_;
	Color		color_;	//!< default will use the global setup color
    };
    
    mStruct DahData
    {
	public:
	    			DahData()
				    : zrg_(mUdf(float),0)
				    , zistime_(false)
				    , dispzinft_(false)		     
				    , markers_(0)
			    	    , d2tm_(0)
				    {}

	const Well::D2TModel*	d2tm_;
	Interval<float>		zrg_;
	bool			dispzinft_;
	bool 			zistime_;
	Well::MarkerSet* 	markers_;
    };
    TypeSet<PickData>&		zPicks()	{ return zpicks_; }
    DahData&			data() 		{ return data_; }
    const DahData&		data() const 	{ return data_; }
    const MouseCursor&		cursor() const 	{ return cursor_; }
    
protected:

    LogData			ld1_;
    LogData			ld2_;

    TypeSet<PickData>		zpicks_;

    Setup			setup_;
    DahData			data_;

    ObjectSet<MarkerItem>	markeritms_;
    ObjectSet<uiLineItem>	zpickitms_;
    
    MouseCursor			cursor_;

    void			init(CallBacker*);
    void			reSized(CallBacker*);

    void			setAxisRelations();
    void			setAxisRanges(bool);
    void			gatherInfo();
    void			gatherInfo(bool);
    void			draw();
    void			drawLog(bool);
    void			drawLine(LogData&,const Well::DahObj* ldah);
    void			drawFilling(bool);
    void			drawMarkers();
    void			drawZPicks();
};


mClass uiWellDisplay : public uiGroup
{
public:

    mStruct Setup
    {
				Setup()
				    : nrpanels_(1)
				    , noborderspace_(false)
				    , nobackground_(false)		   
				    , logwidth_(mLogWidth)	 	   
				    , logheight_(mLogHeight)
				    , withstratdisp_(false) 		    
				    {}

	mDefSetupMemb(int,nrpanels) 	 	// nr Log Panels
	mDefSetupMemb(bool,nobackground) 	//transparent background
	mDefSetupMemb(bool,noborderspace) 	//will remove all border&annots 
	mDefSetupMemb(int,logheight) 		//log height
	mDefSetupMemb(int,logwidth) 		//log width
	mDefSetupMemb(bool,withstratdisp) 	//Add Stratigraphy display
    };

    mClass ShapeSetup
    {
	public:
				    ShapeSetup(uiParent* p)
				    : parent_(p)
				    , withstrat_(false)
				    , nrlogpanels_(1)
				    {}
	uiParent* 	parent_;
	bool 		withstrat_;
	int 		nrlogpanels_;
    };

    mStruct Params
    {
	public :		Params(Well::Data&,int,int);

	int 			logwidth_;	 	   
	int 			logheight_;
	Interval<float>		zrg_;
	bool			zistime_;
	Well::Data&		wd_;
	const Well::D2TModel*	d2tm_;
    };	

				uiWellDisplay(uiParent*,const Setup&,
						Well::Data&);
				uiWellDisplay(uiWellDisplay&,const ShapeSetup&);
				~uiWellDisplay();
    
   
    int 			nrLogDisp() 	{ return logdisps_.size();}	
    uiWellLogDisplay* 		logDisplay(int idx) { return logdisps_[idx]; }
    void			setLog(const char*,int,bool);

    uiWellStratDisplay*		stratDisp() 		{ return stratdisp_; }
    const uiWellStratDisplay*	stratDisp() const  	{ return stratdisp_; }
    bool			hasStratDisp() const	{ return stratdisp_; }
    uiWellDisplayMarkerEdit*	markerEdit() 		{ return mrkedit_; }
    
    Well::Data&			wellData() 		{ return pms_.wd_; }
    const Well::Data&		wellData() const 	{ return pms_.wd_; }
    Params&			params()		{ return pms_; } 
    const Params&		params() const		{ return pms_; } 
    void                        dataChanged(CallBacker*);

    void			setEditOn( bool yn );
    

protected:

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellStratDisplay*		stratdisp_;
    uiWellDisplayMarkerEdit*	mrkedit_;
    Params			pms_;

    void			addLogPanel(bool,bool);
    void			addWDNotifiers(Well::Data&);
    void			setStratDisp();
    void			setInitialZRange();
    int 			getDispWidth();

    void                        gatherInfo();
    void                        setAxisRanges();

    void 			updateProperties(CallBacker*); 
};


mClass uiWellDisplayWin : public uiMainWin
{
public:
    			uiWellDisplayWin(uiParent*,Well::Data&);
			~uiWellDisplayWin(){};

protected:

    uiWellDisplay& 	welldisp_;
    Well::Data&		wd_;
    
    void		closeWin(CallBacker*);
};

#endif
