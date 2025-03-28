#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismultiattribsurvobj.h"

#include "multiid.h"
#include "posinfo2dsurv.h"
#include "seisdatapack.h"
#include "visdrawstyle.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistexturepanelstrip.h"
#include "vistransform.h"
#include "zaxistransform.h"


namespace visSurvey
{

/*!
\brief Used for displaying a 2D line.
*/

mExpClass(visSurvey) Seis2DDisplay : public MultiTextureSurveyObject
{ mODTextTranslationClass(Seis2DDisplay);
public:
				Seis2DDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, Seis2DDisplay,
				    "Seis2DDisplay",
				    ::toUiString(sFactoryKeyword()) )

    void			setGeomID(const Pos::GeomID&);
    BufferString		getLineName() const;
    Pos::GeomID			getGeomID() const override   { return geomid_; }
    MultiID			getMultiID() const override;

    void			setGeometry(const PosInfo::Line2DData&);
    const PosInfo::Line2DData&	getGeometry() const { return geometry_; }

    ZSampling			getMaxZRange(bool displayspace) const;
    void			setZRange(const ZSampling&);
    ZSampling			getZRange(bool displayspace,int att=-1) const;

    void			getTraceKeyPath(TrcKeySet&,
						TypeSet<Coord>*) const override;
    Interval<float>		getDataTraceRange() const override;

    void			setTraceNrRange(const Interval<int>&);
    Interval<int>		getTraceNrRange() const;
    const StepInterval<int>&	getMaxTraceNrRange() const;

    bool			allowsPicks() const override	{ return true; }
    bool			allowMaterialEdit() const override
				{ return true; }
    bool			hasColor() const override	{ return true; }
    OD::Color			getColor() const override;
    void			setColor(OD::Color) override;
    const OD::LineStyle*	lineStyle() const override;
    void			setLineStyle(const OD::LineStyle&) override;

    void			enableAttrib(int attrib,bool yn) override;
    bool			hasSingleColorFallback() const override
				{ return true; }

    void			showPanel(bool yn);
    bool			isPanelShown() const;
    void			showPolyLine(bool yn);
    bool			isPolyLineShown() const;
    void			showLineName(bool yn);
    bool			isLineNameShown() const;

    bool			canDuplicate() const override	{ return true; }
    SurveyObject*		duplicate(TaskRunner*) const override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setPixelDensity(float) override;
    float			getPixelDensity() const override
				{ return pixeldensity_; }

    float			calcDist(const Coord3&) const override;

    int				nrResolutions() const override;
    void			setResolution(int,TaskRunner*) override;

    TrcKeyZSampling		getTrcKeyZSampling(bool displayspace=false,
						int attrib=-1 ) const override;
    OD::Pol2D3D			getAllowedDataType() const override
				{ return OD::Only2D; }
    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const override;
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&,BufferString&,
						uiString&) const override;
    void			getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const override;
    void			getObjectInfo(uiString&) const override;
    void			snapToTracePos(Coord3&) const override;
    int				getNearestTraceNr(const Coord3&) const;

    Coord3			getNearestSubPos(const Coord3& pos,
						 bool usemaxrange) const;
    float			getNearestSegment(const Coord3& pos,
					    bool usemaxrange,int& trcnr1st,
					    int& trcnr2nd,float& frac) const;

    Coord3			projectOnNearestPanel(const Coord3& pos,
						  int* nearestpanelidx=nullptr);
    void			getLineSegmentProjection(
					const Coord3 pos1,const Coord3 pos2,
					TypeSet<Coord3>& projectedcoords);

    Coord			getCoord(int trcnr) const;
    Coord3			getNormal( const Coord3& c ) const override
				{ return SurveyObject::getNormal( c ); }
    Coord			getNormal(int trcnr) const;

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			clearTexture(int);

    void			setAnnotColor(OD::Color) override;
    OD::Color			getAnnotColor() const override;

    visBase::TexturePanelStrip* getTexturePanelStrip();
    const visBase::TexturePanelStrip* getTexturePanelStrip() const;
    const visBase::Text2*	getVisTextLineName() const;

    NotifierAccess*		getMovementNotifier() override
				{ return &geomchanged_; }
    NotifierAccess*		getManipulationNotifier() override
				{ return &geomidchanged_; }

    static Seis2DDisplay*	getSeis2DDisplay(const MultiID&,const char*);
    static Seis2DDisplay*	getSeis2DDisplay(const Pos::GeomID&);

    void			annotateNextUpdateStage(bool yn) override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:
				~Seis2DDisplay();

    void			addCache() override;
    void			removeCache(int) override;
    void			swapCache(int,int) override;
    void			emptyCache(int) override;
    bool			hasCache(int) const override;
    bool			getCacheValue(int attrib,int version,
					      const Coord3&,
					      float&) const override;

    const Interval<int>		getSampleRange() const;

    void			updatePanelStripPath();
				/*!<Sets the coordinates to the path in
				    geometry_, limited by current trcnrrg_.
				    Will also update texture coordinates.*/
    void			updatePanelStripZRange();

    void			updateLineNamePos();
    void			updateTexOriginAndScale(int attrib,
							const TrcKeyZSampling&);
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib,
							  TaskRunner* =nullptr);

    bool			usesDataPacks() const override	{ return true; }
    bool			setVolumeDataPack(int attrib,VolumeDataPack*,
						TaskRunner*) override;
    ConstRefMan<DataPack>	getDataPack(int attrib) const override;
    ConstRefMan<VolumeDataPack> getVolumeDataPack(int attrib) const override;
    RefMan<VolumeDataPack>	getVolumeDataPack(int attrib);
    ConstRefMan<VolumeDataPack> getDisplayedVolumeDataPack(int attrib)
								const override;
    RefMan<VolumeDataPack>	getDisplayedVolumeDataPack(int attrib);

    bool			getNearestTrace(const Coord3&,int& idx,
						float& sqdist) const;
    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool trc,bool z);

    mutable int			prevtrcidx_	= 0;

    RefMan<visBase::PolyLine>	polyline_;
    RefMan<visBase::DrawStyle>	polylineds_;
    RefMan<visBase::TexturePanelStrip> panelstrip_;

    RefObjectSet<RegularSeisDataPack>	datapacks_;
    RefObjectSet<RegularSeisDataPack>	transformedpacks_;

    PosInfo::Line2DData&	geometry_;

    struct TraceDisplayInfo
    {
	TypeSet<int>		alltrcnrs_;
	TypeSet<Coord>		alltrcpos_;
	TypeSet<int>		alljoints_;
	Interval<int>		rg_;
	int			size_;
	ZSampling		zrg_;
    };

    TraceDisplayInfo		trcdisplayinfo_;
    StepInterval<int>		maxtrcnrrg_;

    ConstRefMan<mVisTrans>	transformation_;
    RefMan<visBase::Text2>	linename_;
    Notifier<Seis2DDisplay>	geomchanged_;
    Notifier<Seis2DDisplay>	geomidchanged_;

    Pos::GeomID			geomid_;
    RefMan<ZAxisTransform>	datatransform_;
    int				voiidx_ = -1;
    float			pixeldensity_;

    struct UpdateStageInfo
    {
	float oldtrcrgstart_;
	float oldzrgstart_;
    };
    UpdateStageInfo		updatestageinfo_;

    static const char*		sKeyLineSetID();
    static const char*		sKeyTrcNrRange();
    static const char*		sKeyZRange();
    static const char*		sKeyShowLineName();
    static const char*		sKeyShowPanel();
    static const char*		sKeyShowPolyLine();

				//Old format
    static const char*		sKeyTextureID();

};

} // namespace visSurvey
