#ifndef uiwelllogdisplay_h
#define uiwelllogdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h,v 1.34 2010-04-27 07:22:15 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h" 
#include "uimainwin.h"
#include "uigroup.h"

#include "uiaxishandler.h"
#include "uicursor.h"
#include "draw.h"
#include "multiid.h"
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
				    , withposinfo_(false)		 
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
	mDefSetupMemb(bool,withposinfo)
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
    
    const LogData&		logData(bool first=true) const
    				{ return logData(first); }
    LogData&			logData(bool first=true)
    				{ return first ? ld1_ : ld2_; }

    void 			setDispProperties(Well::DisplayProperties&);
    void			doDataChange()  { dataChanged(); }

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

    TypeSet<PickData>&		zPicks()	{ return zpicks_; }
    Well::Well2DDispData&	data() 		{ return data_; }
    Well::DisplayProperties& 	disp()		{ return disp_; }
    const Well::Well2DDispData&	data() const 	{ return data_; }
    const MouseCursor&		cursor() const 	{ return cursor_; }
    float			mousePos(); 
    
    Notifier<uiWellLogDisplay>  infoChanged;
    
protected:

    LogData			ld1_;
    LogData			ld2_;

    TypeSet<PickData>		zpicks_;

    Setup			setup_;
    Well::Well2DDispData	data_;
    Well::DisplayProperties 	disp_;
    
    ObjectSet<MarkerItem>	markeritms_;
    ObjectSet<uiLineItem>	zpickitms_;
    
    MouseCursor			cursor_;

    void			init(CallBacker*);
    void			dataChanged();
    void			reSized(CallBacker*);
    void			mouseMoved(CallBacker*);

    void			setAxisRelations();
    void			setAxisRanges(bool);
    void			gatherInfo();
    void			gatherInfo(bool);
    void 			getPosInfo(float,BufferString&);
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
				    , withedit_(false)	       
				    , wd_(0)				    
				    {}

	mDefSetupMemb(Well::Data*,wd) 		// will be used if not null
	mDefSetupMemb(int,nrpanels) 	 	// nr Log Panels
	mDefSetupMemb(bool,nobackground) 	//transparent background
	mDefSetupMemb(bool,noborderspace) 	//will remove all border&annots 
	mDefSetupMemb(int,logheight) 		//log height
	mDefSetupMemb(int,logwidth) 		//log width
	mDefSetupMemb(bool,withstratdisp) 	//Add Stratigraphy display
	mDefSetupMemb(bool,withedit) 		//Add Marker Editor
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
	public :		Params(Well::Data*,int,int);

	int 			logwidth_;	 	   
	int 			logheight_;
	Well::Data*		wd_;
	Well::Well2DDispData	data_;
    };	
				uiWellDisplay(uiParent*,const Setup&,
					      const MultiID& );
				uiWellDisplay(uiWellDisplay&,const ShapeSetup&);
				~uiWellDisplay();
    
    void			setLog(const char*,int,bool);
    int 			nrLogDisp() 	  { return logdisps_.size();}	
    int 			nrLogDisp() const { return logdisps_.size();}	
    uiWellLogDisplay* 		logDisplay(int i) 	{ return logdisps_[i]; }
    const uiWellLogDisplay* 	logDisplay(int i) const	{ return logdisps_[i]; }

    MultiID                     getWellID() 		{ return wellid_; }
    uiWellStratDisplay*		stratDisp() 		{ return stratdisp_; }
    const uiWellStratDisplay*	stratDisp() const  	{ return stratdisp_; }
    bool			hasStratDisp() const	{ return stratdisp_; }
    uiWellDisplayMarkerEdit* 	markerEdit() 		{ return mrkedit_; }
    const uiWellDisplayMarkerEdit* markerEdit() const	{ return mrkedit_; }
   
    void 			setDispProperties(Well::DisplayProperties&,
						    int panelnr=0); 
    
    Well::Data*			wellData() const 	{ return getWD(); }
    Params&			params()		{ return pms_; } 
    const Params&		params() const		{ return pms_; } 
    void                        dataChanged(CallBacker*);

    BufferString		getPosInfo() const	{ return info_; } 

protected:

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellStratDisplay*		stratdisp_;
    uiWellDisplayMarkerEdit*	mrkedit_;
    Params			pms_;
    BufferString		info_;
    MultiID                     wellid_;

    void			addLogPanel(bool,bool);
    void			addWDNotifiers(Well::Data&);
    Well::Data*			getWD() const;
    void			removeWDNotifiers(Well::Data&);
    void 			setPosInfo(CallBacker*);
    void			setStratDisp();
    int 			getDispWidth();

    void                        gatherInfo();
    void                        setAxisRanges();

    void			welldataDelNotify(CallBacker*);
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
    void		updateProperties(CallBacker*);
};

#endif
