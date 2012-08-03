#ifndef uiwelldahdisplay_h
#define uiwelldahdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2010
 RCS:           $Id: uiwelldahdisplay.h,v 1.17 2012-08-03 13:01:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiwellmod.h"
#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uiaxishandler.h"
#include "draw.h"
#include "survinfo.h"
#include "welldisp.h"
#include "welldata.h"
#include "welltrack.h"

namespace Well { class DahObj; class Marker; class D2TModel; }

class uiTextItem;
class uiLineItem;
class uiPolyLineItem;
class uiPolygonItem;
class uiGraphicsScene;
class UnitOfMeasure;



#define mDefZPos(zpos)\
if ( zdata_.zistime_ && zdata_.d2T() )\
    zpos = d2T()->getTime( zpos )*SI().zDomain().userFactor();\
else if ( !zdata_.zistime_ && track() )\
    zpos = track() ? zdata_.track()->getPos( zpos ).z : 0;

#define mDefZPosInLoop(val) \
    float zpos = val;\
    mDefZPos(zpos)\
    if ( !ld1_->yax_.range().includes( zpos, true ) )\
	continue;

mClass(uiWell) uiWellDahDisplay : public uiGraphicsView
{
public:	
    mStruct(uiWell) Setup
    {
			    Setup()
			    : nrmarkerchars_(2)
			    , pickls_(LineStyle::Solid,1,Color(0,200,0))
			    , border_(5)
			    , noxannot_(false)
			    , noyannot_(false)
			    , annotinside_(false)
			    , samexaxisrange_(false)
			    , symetricalxaxis_(false) 
			    , drawcurvenames_(false)
			    , xannotinpercents_(false)			   
			    {}

	mDefSetupMemb(uiBorder,border)
	mDefSetupMemb(int,nrmarkerchars)  //!< Will display up to this nr chars
	mDefSetupMemb(LineStyle,pickls)   //!< color used if no PickData color
	mDefSetupMemb(int,axisticsz)
	mDefSetupMemb(bool,noxannot)
	mDefSetupMemb(bool,noyannot)
	mDefSetupMemb(bool,annotinside)
	mDefSetupMemb(bool,drawcurvenames)
	mDefSetupMemb(bool,samexaxisrange)
	mDefSetupMemb(bool,symetricalxaxis)
	mDefSetupMemb(bool,xannotinpercents)
    };

				    uiWellDahDisplay(uiParent*,const Setup&);
				    ~uiWellDahDisplay();

    mStruct(uiWell) DahObjData
    {
	virtual			~DahObjData() { delete xaxprcts_; }

	//Set these	
	void			setData(const Well::DahObj* d) { dahobj_ = d; }
	bool                    xrev_;
	int                     zoverlayval_;
	float			cliprate_;
	Color			col_;
	bool			drawascurve_;
	int		 	curvesz_;	
	bool			drawaspoints_;
	int		 	pointsz_;	

	//Get these
	Interval<float>         zrg_;
	Interval<float>         valrg_;
	uiAxisHandler           xax_;
	uiAxisHandler*          xaxprcts_;
	uiAxisHandler           yax_;

	virtual void		getInfoForDah(float dah,BufferString&) const;
	void			plotAxis();

    protected:
				DahObjData(uiGraphicsScene&,bool,
					const uiWellDahDisplay::Setup&);

	const Well::DahObj*	dahobj_;
	uiGraphicsItemSet       curveitms_;
	uiPolyLineItem*         curvepolyitm_;

	friend class            uiWellDahDisplay;
    };

    mStruct(uiWell) Data
    {
				    Data()
				    : zrg_(mUdf(float),mUdf(float))
				    , zistime_(SI().zIsTime())
				    , dispzinft_(SI().depthsInFeetByDefault())
				    , wd_(0)
				    {}

	void copyFrom(const uiWellDahDisplay::Data& d)
	{
	    zrg_     	= d.zrg_;
	    zistime_ 	= d.zistime_;
	    dispzinft_ 	= d.dispzinft_;
	    wd_ 	= d.wd_;
	}
	const Well::D2TModel*	d2T() const { return wd_ ? wd_->d2TModel() : 0;}
	const Well::Track*	track() const {return wd_ ? &wd_->track() : 0; }
	const Well::MarkerSet*	mrks() const {return wd_ ? &wd_->markers() : 0;}

	Interval<float>		zrg_;
	bool			dispzinft_;
	bool			zistime_;
	const Well::Data*	wd_;
    };

    mStruct(uiWell) PickData
    {
				PickData( float dah, Color c=Color::NoColor() )
				    : dah_(dah), color_(c), val_(mUdf(float)) {}

	bool                    operator ==( const PickData& pd ) const
				{ return mIsEqual(pd.dah_,dah_,1e-4); }

	float                   dah_;
	Color                   color_; //!< default will use the global 
					//setup color
	float			val_; //this will be a point if defined, 
				      //a line otherwise
    };

    void			setData(const Data& data)
					{ zdata_.copyFrom(data); dataChanged();}
    void			setZRange(Interval<float> zrg)
					{ zdata_.zrg_ = zrg; dataChanged();}

    const Data&			zData() 	   { return zdata_; }
    TypeSet<PickData>&          zPicks()           { return zpicks_; }
    Well::DisplayProperties::Markers& markerDisp() { return  mrkdisp_; }

    void			reDraw()	{ gatherInfo(); draw(); }
    void			reDrawAnnots()	{ drawMarkers(); drawZPicks(); }

    DahObjData&                 dahObjData( bool first ) 
    				{ return first ? *ld1_ : *ld2_; }
protected:

    DahObjData*			ld1_;
    DahObjData*			ld2_;
    Data			zdata_;
    Setup                       setup_;
    TypeSet<PickData>           zpicks_;
    uiGraphicsItemSet       	zpickitms_;

    mStruct(uiWell) MarkerDraw
    {
			    MarkerDraw( const Well::Marker& mrk )
				: mrk_(mrk)
				{}
			    ~MarkerDraw();

	const Well::Marker&     mrk_;
	LineStyle               ls_;
	uiTextItem*             txtitm_;
	uiLineItem*             lineitm_;
    };
    ObjectSet<MarkerDraw>       markerdraws_;
    MarkerDraw*                 getMarkerDraw(const Well::Marker&);
    Well::DisplayProperties::Markers mrkdisp_;

    const Well::D2TModel*	d2T() const 	{ return zdata_.d2T(); }
    const Well::Track*		track() const 	{ return zdata_.track(); }
    const Well::MarkerSet*	markers() const { return zdata_.mrks(); }

    virtual void		draw();
    virtual void		drawCurve(bool);
    void                        drawMarkers();
    void                        drawZPicks();

    void			setAxisRelations();
    virtual void		gatherInfo();
    virtual void		gatherDataInfo(bool);
    void			setAxisRanges(bool);

    void			dataChanged();
    void			init(CallBacker*);
    void			reSized(CallBacker*);

    friend class                uiWellDisplay;
    friend class                uiWellDisplayControl;
};


#endif


