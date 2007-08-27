#ifndef visseis2ddisplay_h
#define visseis2ddisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visseis2ddisplay.h,v 1.16 2007-08-27 10:04:19 cvsnanne Exp $
________________________________________________________________________


-*/


#include "vismultiattribsurvobj.h"
#include "multiid.h"

class SeisTrcInfo;
class ZAxisTransform;

namespace visBase
{
    class Text2;
    class Transformation;
    class TriangleStripSet;
}

namespace Attrib  { class Data2DHolder; }
namespace PosInfo { class Line2DData; }

namespace visSurvey
{

/*!\brief Used for displaying a 2D line

*/

class Seis2DDisplay : public MultiTextureSurveyObject
{
public:
    static Seis2DDisplay*	create()
				mCreateDataObj(Seis2DDisplay);

    void			setLineName(const char*);

    void			setGeometry(const PosInfo::Line2DData&);
    StepInterval<float>		getMaxZRange(bool displayspace) const;
    bool			setZRange(const Interval<float>&);
    				//!<\returns whether a cache update is enough.
    Interval<float>		getZRange(bool displayspace) const;

    bool			setTraceNrRange(const Interval<int>&);
    				//!<\returns whether a cache update is enough.
    const Interval<int>&	getTraceNrRange() const;
    const Interval<int>&	getMaxTraceNrRange() const;

    bool			setDataPackID(int attrib,DataPack::ID);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const	{ return 1; }

    void			setTraceData(int attrib,
	    				     const Attrib::Data2DHolder&);
    const Attrib::Data2DHolder*	getCache(int attrib) const;
    void			updateDataFromCache();

    bool			allowPicks() const		{ return true; }
    bool			allowMaterialEdit() const	{ return true; }

    void			showLineName(bool);
    bool			lineNameShown() const;

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate() const;

    void			setDisplayTransformation(
	    					visBase::Transformation*);
    visBase::Transformation*	getDisplayTransformation();

    float			calcDist(const Coord3&) const;

    int				nrResolutions() const		{ return 3; }
    int				getResolution() const;
    void			setResolution(int);

    Pol2D3D                     getAllowedDataType() const	{return Only2D;}
    SurveyObject::AttribFormat 	getAttributeFormat() const;
    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,BufferString&,
	    					BufferString&) const;
    void			getObjectInfo(BufferString&) const;
    void			snapToTracePos(Coord3&);

    void			setLineSetID(const MultiID& mid);
    const MultiID&		lineSetID() const;

    Coord			getCoord(int trcnr) const;

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;

    void			clearTexture(int);

    NotifierAccess*		getMovementNotifier()
    				{ return &geomchanged_; }

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

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
    void			updateVizPath();
    				/*!<Sets the coordinates to the path in
				    geometry_, limited by the current
				    trcnrrg_. Will also update the
				    z-coordinates & texture coordinates.*/

    void			setStrip(const TypeSet<Coord>&,
	    				 const Interval<float>&,
					 int stripidx,const Interval<int>&);
    void			updateLineNamePos();
    void			setData(int attrib,const Attrib::Data2DHolder&);
    bool			getNearestTrace(const Coord3&,int& idx,
						float& sqdist) const;
    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool trc,bool z);

    ObjectSet<const Attrib::Data2DHolder>	cache_;
    TypeSet<DataPack::ID>			datapackids_;
    MultiID					linesetid_;

    PosInfo::Line2DData&			geometry_;
    Interval<float>				curzrg_;
    Interval<int>				trcnrrg_;
    Interval<int>				maxtrcnrrg_;

    ObjectSet<visBase::TriangleStripSet>	planes_;

    visBase::Transformation*			transformation_;
    visBase::Text2*				linename_;
    Notifier<Seis2DDisplay>			geomchanged_;

    ZAxisTransform*				datatransform_;
    int						voiidx_;

    static const char*				sKeyLineSetID();
    static const char*				sKeyTrcNrRange();
    static const char*				sKeyZRange();
    static const char*				sKeyShowLineName();

						//Old format
    static const char*				sKeyTextureID();
};

} // namespace visSurvey


#endif
