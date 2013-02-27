#ifndef visseis2ddisplay_h
#define visseis2ddisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id$
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "vismultiattribsurvobj.h"
#include "multiid.h"
#include "surv2dgeom.h"

class SeisTrcInfo;
class ZAxisTransform;
class Color;

namespace visBase
{
    class Text2;
    class Transformation;
    class SplitTextureSeis2D;
}

namespace Attrib  { class Data2DArray; }
namespace PosInfo { class Line2DData; }

namespace visSurvey
{

/*!
\brief Used for displaying a 2D line.
*/

mExpClass(visSurvey) Seis2DDisplay : public MultiTextureSurveyObject
{
public:
    static Seis2DDisplay*	create()
				mCreateDataObj(Seis2DDisplay);

    void			setLineInfo(const MultiID& lid,const char* lnm);
    const char*			getLineName() const;
    const MultiID&		lineSetID() const;
    PosInfo::GeomID		getGeomID() const;
    TraceID::GeomID		geomID() const;
    MultiID			getMultiID() const;

    void			setGeometry(const PosInfo::Line2DData&);
    const PosInfo::Line2DData&	getGeometry() const { return geometry_; }

    StepInterval<float>		getMaxZRange(bool displayspace) const;
    void				setZRange(const StepInterval<float>&);
    StepInterval<float>		getZRange(bool displayspace,int att=-1) const;

    void			setTraceNrRange(const Interval<int>&);
    Interval<int>		getTraceNrRange() const;
    const StepInterval<int>&	getMaxTraceNrRange() const;

    bool			setDataPackID(int attrib,DataPack::ID,
								   TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
						{return DataPackMgr::FlatID(); }

    void			setTraceData(int attrib,
	    			const Attrib::Data2DArray&,TaskRunner*);
    virtual void		setTraceData( int attrib, SeisTrcBuf& tb,
				TaskRunner* tr )
				{ SurveyObject::setTraceData(attrib,tb,tr); }

    const Attrib::Data2DArray*	getCache(int attrib) const;
    void			updateDataFromCache(TaskRunner*);

    bool			allowsPicks() const		{ return true; }
    bool			allowMaterialEdit() const	{ return true; }

    void			showLineName(bool);
    bool			lineNameShown() const;

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate(TaskRunner*) const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    float			calcDist(const Coord3&) const;

    int				nrResolutions() const;
    int				getResolution() const;
    void			setResolution(int,TaskRunner*);

    Pol2D3D                     getAllowedDataType() const	{return Only2D;}
    SurveyObject::AttribFormat 	getAttributeFormat(int attrib) const;
    void			getMousePosInfo(const visBase::EventInfo&,
				    Coord3&,BufferString&,BufferString&) const;
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

    Coord			getCoord(int trcnr) const;
    virtual Coord3		getNormal( const Coord3& c ) const
					{ return SurveyObject::getNormal( c ); }
    Coord			getNormal(int trcnr) const;

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    void			clearTexture(int);

    virtual void		setAnnotColor(Color);
    virtual Color		getAnnotColor() const;
    
    NotifierAccess*		getMovementNotifier() { return &geomchanged_; }

    static Seis2DDisplay*	getSeis2DDisplay(const MultiID&,const char*);

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
				~Seis2DDisplay();
    friend class Seis2DTextureDataArrayFiller;
    friend class Seis2DArray;
    
    virtual void		addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;
    bool			getCacheValue(int attrib,int version,
						const Coord3&,float&) const;

    const Interval<int>		getSampleRange() const;
    void			updateVizPath();
    				/*!<Sets the coordinates to the path in
				geometry_, limited by the current
				trcnrrg_. Will also update the
				z-coordinates & texture coordinates.*/

    void			updateLineNamePos();
    void			setData(int attrib,const Attrib::Data2DArray&,
	    							TaskRunner*);
    bool			getNearestTrace(const Coord3&,int& idx,
							float& sqdist) const;
    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool trc,bool z);

    mutable int			prevtrcidx_;

    visBase::SplitTextureSeis2D* triangles_;
    ObjectSet<const Attrib::Data2DArray> cache_;
    TypeSet<DataPack::ID>	datapackids_;
    MultiID			linesetid_;

    PosInfo::Line2DData&	geometry_;

    struct TraceDisplayInfo
    {
	TypeSet<int>		alltrcnrs;
	Interval<int>		rg;
	int			size;
	StepInterval<float>	zrg;
    };
    TraceDisplayInfo		trcdisplayinfo_;
    StepInterval<int>		maxtrcnrrg_;

    const mVisTrans*		transformation_;
    visBase::Text2*		linename_;
    Notifier<Seis2DDisplay>	geomchanged_;

    PosInfo::GeomID		oldgeomid_;
    TraceID::GeomID		geomid_;
    ZAxisTransform*		datatransform_;
    int				voiidx_;

    static const char*		sKeyLineSetID();
    static const char*		sKeyTrcNrRange();
    static const char*		sKeyZRange();
    static const char*		sKeyShowLineName();

				//Old format
    static const char*		sKeyTextureID();
};

} // namespace visSurvey


#endif

