#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uiaxishandler.h"
#include "draw.h"
#include "survinfo.h"
#include "welldisp.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "survinfo.h"

namespace Well { class DahObj; class Marker; class D2TModel; }

class uiTextItem;
class uiLineItem;
class uiPolyLineItem;
class uiPolygonItem;
class uiGraphicsScene;

#define mDefZPos(zpos)\
if ( zdata_.zistime_ && zdata_.d2T() && track() )\
    zpos = d2T()->getTime( zpos, *track() )*SI().zDomain().userFactor();\
else if ( !zdata_.zistime_ && track() )\
    zpos = track() ? (float) zdata_.track()->getPos( zpos ).z : 0;

#define mDefZPosInLoop(val) \
    float zpos = val;\
    if ( zdata_.zistime_ && zdata_.d2T() && track() )\
    zpos = d2T()->getTime( zpos, *track() )*SI().zDomain().userFactor();\
    else if ( !zdata_.zistime_ )\
    {\
	if ( track() )\
	    zpos = (float) zdata_.track()->getPos( zpos ).z; \
	const UnitOfMeasure* zdum = UnitOfMeasure::surveyDefDepthUnit();\
	const UnitOfMeasure* zsum = UnitOfMeasure::surveyDefDepthStorageUnit();\
	zpos = getConvertedValue( zpos, zsum, zdum );\
    }\
    if ( !ld1_->yax_.range().includes( zpos, true ) )\
	continue;

/*!
\brief Well depth/distance along hole display.
*/

mExpClass(uiWell) uiWellDahDisplay : public uiGraphicsView
{ mODTextTranslationClass(uiWellDahDisplay)
public:
    mStruct(uiWell) Setup
    {
			    Setup()
			    : nrmarkerchars_(2)
			    , pickls_(OD::LineStyle::Solid,1,OD::Color(0,200,0))
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
	mDefSetupMemb(OD::LineStyle,pickls) //!< color used if no PickData color
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

    mStruct(uiWell) DahObjData : public CallBacker
    {
	virtual			~DahObjData() { delete xaxprcts_; }

	//Set these
	void			setData(const Well::DahObj* d) { dahobj_ = d; }
	bool			hasData() const { return dahobj_; }
	bool			xrev_;
	int			zoverlayval_;
	float			cliprate_;
	OD::Color		col_;
	bool			drawascurve_;
	int			curvesz_;
	bool			drawaspoints_;
	int			pointsz_;

	//Get these
	Interval<float>		zrg_;
	Interval<float>		valrg_;
	uiAxisHandler		xax_;
	uiAxisHandler*		xaxprcts_;
	uiAxisHandler		yax_;

	virtual void		getInfoForDah(float dah,BufferString&) const;
	void			plotAxis();

    protected:
				DahObjData(uiGraphicsScene&,bool,
					const uiWellDahDisplay::Setup&);

	const Well::DahObj*	dahobj_;
	uiGraphicsItemSet	curveitms_;
	uiPolyLineItem*		curvepolyitm_;

	friend class		uiWellDahDisplay;
    };

    mStruct(uiWell) Data
    {
				    Data( const Well::Data* wd )
				    : zrg_(mUdf(float),mUdf(float))
				    , zistime_(false)
				    , dispzinft_(SI().depthsInFeet())
				    , wd_(wd)
				    { if ( wd_ ) wd_->ref(); }
				    ~Data()
				    { if ( wd_ ) wd_->unRef(); }

	void copyFrom(const uiWellDahDisplay::Data& d)
	{
	    if ( &d == this )
		return;

	    zrg_     	= d.zrg_;
	    zistime_ 	= d.zistime_;
	    dispzinft_ 	= d.dispzinft_;
	    if ( wd_ )
		wd_->unRef();
	    wd_ 	= d.wd_;
	    if ( wd_ )
		wd_->ref();
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
				PickData( float dah,
					    OD::Color c=OD::Color::NoColor() )
				    : dah_(dah), color_(c), val_(mUdf(float)) {}

	bool			operator ==( const PickData& pd ) const
				{ return mIsEqual(pd.dah_,dah_,1e-4); }

	float			dah_;
	OD::Color		color_; //!< default will use the global
					//setup color
	float			val_; //this will be a point if defined,
				      //a line otherwise
    };

    void			setData(const Data& data);
    void			setZRange(Interval<float> zrg)
					{ zdata_.zrg_ = zrg; dataChanged();}

    const Data&			zData() 		{ return zdata_; }
    TypeSet<PickData>&		zPicks()		{ return zpicks_; }
    Well::DisplayProperties::Markers& markerDisp()	{ return  mrkdisp_; }

    void			reDraw()	{ gatherInfo(); draw(); }
    void			reDrawAnnots()	{ drawMarkers(); drawZPicks(); }

    DahObjData&			dahObjData( bool first )
				{ return first ? *ld1_ : *ld2_; }
protected:

    DahObjData*			ld1_;
    DahObjData*			ld2_;
    Data			zdata_;
    Setup			setup_;
    TypeSet<PickData>		zpicks_;
    uiGraphicsItemSet		zpickitms_;

    mStruct(uiWell) MarkerDraw
    {
				MarkerDraw( const Well::Marker& mrk )
				: mrk_(mrk)
				{}
				~MarkerDraw();

	const Well::Marker&	mrk_;
	OD::LineStyle		ls_;
	uiTextItem*		txtitm_ = nullptr;
	uiLineItem*		lineitm_ = nullptr;
    };

    ObjectSet<MarkerDraw>	markerdraws_;
    MarkerDraw*			getMarkerDraw(const Well::Marker&);
    Well::DisplayProperties::Markers mrkdisp_;

    const Well::D2TModel*	d2T() const 	{ return zdata_.d2T(); }
    const Well::Track*		track() const 	{ return zdata_.track(); }
    const Well::MarkerSet*	markers() const { return zdata_.mrks(); }

    virtual void		draw();
    virtual void		drawCurve(bool);
    void			drawMarkers();
    void			drawZPicks();

    void			setAxisRelations();
    virtual void		gatherInfo();
    virtual void		gatherDataInfo(bool);
    void			setAxisRanges(bool);

    void			dataChanged();
    void			init(CallBacker*);
    void			reSized(CallBacker*);

    friend class		uiWellDisplay;
    friend class		uiWellDisplayControl;
};
