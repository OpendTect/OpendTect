#ifndef uiwelllogdisplay_h
#define uiwelllogdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h
________________________________________________________________________

-*/


#include "uiwelldahdisplay.h"
#include "uiaxishandler.h"
#include "draw.h"

class uiTextItem;
class uiLineItem;
class uiPolyLineItem;
class uiPolygonItem;
class uiGraphicsScene;
class UnitOfMeasure;

namespace Well { class Log; class Marker; }


/*!\brief creates a display of max 2 well logs. */
mClass uiWellLogDisplay : public uiWellDahDisplay
{
public:

    mStruct Setup
    {
			    Setup()
			    : nrmarkerchars_(2)
			    , pickls_(LineStyle::Solid,1,Color(0,200,0))
			    , border_(5)
			    , noxannot_(false)
			    , noyannot_(false)      		     
			    , annotinside_(false)
			    , sameaxisrange_(false)			 
			    {}

	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(int,nrmarkerchars)  //!< Will display up to this nr chars
	mDefSetupMemb(LineStyle,pickls)   //!< color used if no PickData color
	mDefSetupMemb(int,axisticsz)
	mDefSetupMemb(bool,noxannot)
	mDefSetupMemb(bool,noyannot)
	mDefSetupMemb(bool,annotinside)
	mDefSetupMemb(bool,sameaxisrange)
    };  

			    uiWellLogDisplay(uiParent*,const Setup&);
			    ~uiWellLogDisplay();

    mStruct LogData
    {
	// Set these
	const Well::Log*        wl_;
	const UnitOfMeasure*    unitmeas_;
	bool                    xrev_;
	Well::DisplayProperties::Log disp_;
	int			zoverlayval_;

	// Get these (will be filled)
	Interval<float>         zrg_;
	Interval<float>         valrg_;
	uiAxisHandler           xax_;
	uiAxisHandler           yax_;
	uiPolyLineItem* 	curveitm_;
	ObjectSet<uiPolygonItem> curvepolyitms_;
	uiTextItem*             curvenmitm_;

    protected:

				LogData(uiGraphicsScene&,bool isfirst,
					const uiWellLogDisplay::Setup&);
	void                    copySetupFrom( const LogData& ld )
				{
				    unitmeas_   = ld.unitmeas_;
				    xrev_       = ld.xrev_;
				    disp_       = ld.disp_;
				}

	friend class            uiWellLogDisplay;
    };

    LogData&                    logData( bool first=true )
				{ return first ? ld1_ : ld2_; }
    const LogData&              logData( bool first=true ) const
				{ return first ? ld1_ : ld2_; }

    mStruct PickData
    {
				PickData( float dah, Color c=Color::NoColor() )
				    : dah_(dah), color_(c)      {}
	bool            	operator ==( const PickData& pd ) const
	    			{ return mIsEqual(pd.dah_,dah_,1e-4); }

	float           	dah_;
	Color           	color_; //!< default will use the global 
					//setup color
    };
    TypeSet<PickData>&          zPicks()        { return zpicks_; }

    Well::DisplayProperties::Markers& markerDisp() { return  mrkdisp_; }

protected:

    LogData                     ld1_;
    LogData                     ld2_;
    Setup                       setup_;

    TypeSet<PickData>           zpicks_;
    ObjectSet<uiLineItem>       zpickitms_;

    mStruct MarkerDraw
    {
				MarkerDraw( const Well::Marker& mrk )
				: mrk_(mrk)
				{}
			        ~MarkerDraw();

	const Well::Marker& 	mrk_;
	LineStyle		ls_;
	uiTextItem*     	txtitm_;
	uiLineItem*     	lineitm_;
    };
    ObjectSet<MarkerDraw>       markerdraws_;
    MarkerDraw*                 getMarkerDraw(const Well::Marker&);
    Well::DisplayProperties::Markers mrkdisp_;

    void                        gatherInfo();
    void                        draw();

    void                        setAxisRelations();
    void                        gatherInfo(bool);
    void                        setAxisRanges(bool);
    void                        drawCurve(bool);
    void                        drawSeismicCurve(bool);
    void                        drawFilledCurve(bool);
    void                        drawMarkers();
    void                        drawZPicks();

    friend class                uiWellDisplay;
    friend class                uiWellDisplayControl;
};

#endif

