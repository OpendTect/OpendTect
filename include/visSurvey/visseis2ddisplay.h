#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2003
________________________________________________________________________


-*/

#include "vissurveycommon.h"
#include "vismultiattribsurvobj.h"
#include "posinfo2dsurv.h"
#include "probe.h"
#include "seisdatapack.h"

namespace visBase
{
    class DrawStyle;
    class PolyLine;
    class Text;
    class TexturePanelStrip;
}

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
				    SurveyObject,Seis2DDisplay,
				    "Seis2DDisplay",
				    toUiString(sFactoryKeyword()))
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    void			setProbe(Probe*);
    Probe*			getProbe()		{ return probe_; }
    const Probe*		getProbe() const	{ return probe_; }
    BufferString		getLineName() const;
    Pos::GeomID			getGeomID() const;

    void			setGeometry(const PosInfo::Line2DData&);
    const PosInfo::Line2DData&	getGeometry() const { return geometry_; }

    StepInterval<float>		getMaxZRange(bool displayspace) const;
    void			setZRange(const StepInterval<float>&);
    StepInterval<float>		getZRange(bool displayspace,int att=-1) const;

    void			getTraceKeyPath(TrcKeyPath&,
                                                TypeSet<Coord>*) const;
    Interval<float>		getDataTraceRange() const;

    void			setTraceNrRange(const Interval<int>&);
    const Interval<int>&	getTraceNrRange() const;
    const StepInterval<int>&	getMaxTraceNrRange() const;

    bool			setDataPackID(int attrib,DataPack::ID,
					      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPack::ID		getDisplayedDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
				{ return DataPackMgr::SeisID(); }

    bool			allowsPicks() const		{ return true; }
    bool			allowMaterialEdit() const	{ return true; }
    bool			hasColor() const		{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    const OD::LineStyle*		lineStyle() const;
    void			setLineStyle(const OD::LineStyle&);

    void			enableAttrib(int attrib,bool yn);
    bool			hasSingleColorFallback() const	{ return true; }

    void			showPanel(bool yn);
    bool			isPanelShown() const;
    void			showPolyLine(bool yn);
    bool			isPolyLineShown() const;
    void			showLineName(bool yn);
    bool			isLineNameShown() const;

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate(TaskRunner*) const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setPixelDensity(float);
    float			getPixelDensity() const {return pixeldensity_;}

    float			calcDist(const Coord3&) const;

    int				nrResolutions() const;
    void			setResolution(int,TaskRunner*);

    TrcKeyZSampling		getTrcKeyZSampling( int attrib=-1 ) const
				{ return getTrcKeyZSampling( false, attrib ); }
    TrcKeyZSampling		getTrcKeyZSampling(bool displayspace,
						   int attrib=-1) const;
    OD::Pol2D3D			getAllowedDataType() const
				{ return OD::Only2D; }
    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const;
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&,BufferString&,
						BufferString&) const;
    void			getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const;
    void			getObjectInfo(BufferString&) const;
    void			snapToTracePos(Coord3&) const;
    int				getNearestTraceNr(const Coord3&) const;

    Coord3			getNearestSubPos(const Coord3& pos,
						 bool usemaxrange) const;
    float			getNearestSegment(const Coord3& pos,
					    bool usemaxrange,int& trcnr1st,
					    int& trcnr2nd,float& frac ) const;

    Coord3			projectOnNearestPanel(const Coord3& pos,
						      int* nearestpanelidx=0);
    void			getLineSegmentProjection(
					const Coord3 pos1,const Coord3 pos2,
					TypeSet<Coord3>& projectedcoords);

    Coord			getCoord(int trcnr) const;
    virtual Coord3		getNormal( const Coord3& c ) const
				{ return SurveyObject::getNormal( c ); }
    Coord			getNormal(int trcnr) const;

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    void			clearTexture(int);

    virtual void		setAnnotColor(Color);
    virtual Color		getAnnotColor() const;

    NotifierAccess*		getMovementNotifier()
				{ return &geomchanged_; }
    NotifierAccess*		getManipulationNotifier()
				{ return &geomidchanged_; }

    static Seis2DDisplay*	getSeis2DDisplay(const DBKey&,const char*);
    static Seis2DDisplay*	getSeis2DDisplay(Pos::GeomID);

    virtual void		annotateNextUpdateStage(bool yn);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    visBase::TexturePanelStrip* getTexturePanelStrip() const
				{ return panelstrip_; }
    bool			isSection() const { return true; }

    const visBase::Text*	getVisTextLineName() { return linename_; }

protected:
				~Seis2DDisplay();

    virtual void		addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;
    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;

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
							  TaskRunner* =0);
    bool			getNearestTrace(const Coord3&,int& idx,
						float& sqdist) const;
    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool trc,bool z);

    mutable int			prevtrcidx_;

    visBase::PolyLine*		polyline_;
    visBase::DrawStyle*		polylineds_;
    visBase::TexturePanelStrip* panelstrip_;

    RefObjectSet<RegularSeisDataPack>	datapacks_;
    RefObjectSet<RegularSeisDataPack>	transformedpacks_;

    DBKey			datasetid_;

    PosInfo::Line2DData&	geometry_;

    struct TraceDisplayInfo
    {
	TypeSet<int>		alltrcnrs_;
	TypeSet<Coord>		alltrcpos_;
	TypeSet<int>		alljoints_;
	Interval<int>		rg_;
	int			size_;
	StepInterval<float>	zrg_;
    };

    TraceDisplayInfo		trcdisplayinfo_;
    StepInterval<int>		maxtrcnrrg_;

    const mVisTrans*		transformation_;
    visBase::Text*		linename_;
    Notifier<Seis2DDisplay>	geomchanged_;
    Notifier<Seis2DDisplay>	geomidchanged_;

    RefMan<Probe>		probe_;
    ZAxisTransform*		datatransform_;
    int				voiidx_;
    float			pixeldensity_;

    struct UpdateStageInfo
    {
	int	oldtrcrgstart_;
	float	oldzrgstart_;
    };
    UpdateStageInfo		updatestageinfo_;

    static const char*		sKeyLineSetID();
    static const char*		sKeyTrcNrRange();
    static const char*		sKeyZRange();
    static const char*		sKeyShowLineName();
    static const char*		sKeyShowPanel();
    static const char*		sKeyShowPolyLine();
};

} // namespace visSurvey
