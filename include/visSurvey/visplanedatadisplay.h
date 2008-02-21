#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.102 2008-02-21 11:13:14 cvsnanne Exp $
________________________________________________________________________


-*/


#include "vismultiattribsurvobj.h"
#include "ranges.h"

template <class T> class Array2D;
class ZAxisTransformer;

namespace visBase
{
    class Coordinates;
    class DepthTabPlaneDragger;
    class DrawStyle;
    class FaceSet;
    class GridLines;
    class PickStyle;
};


namespace Attrib { class DataCubes; }

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use <code>setOrientation(Orientation)</code> for
    setting the requested orientation of the slice.
*/

class PlaneDataDisplay :  public visSurvey::MultiTextureSurveyObject
{
public:

    bool			isInlCrl() const	{ return true; }

    enum Orientation		{ Inline=0, Crossline=1, Timeslice=2 };
    				DeclareEnumUtils(Orientation);

    static PlaneDataDisplay*	create()
				mCreateDataObj(PlaneDataDisplay);

    void			setOrientation(Orientation);
    Orientation			getOrientation() const { return orientation_; }

    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const	{ return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;
    NotifierAccess*		getManipulationNotifier();
    NotifierAccess*		getMovementNotifier()
    				{ return &movefinished_; }

    bool			allowMaterialEdit() const	{ return true; }

    int				nrResolutions() const;
    void			setResolution(int);

    SurveyObject::AttribFormat	getAttributeFormat() const;

    CubeSampling		getCubeSampling(int attrib=-1) const;
    CubeSampling		getCubeSampling(bool manippos,
	    					bool displayspace,
						int attrib=-1) const;
    void			getRandomPos( ObjectSet<BinIDValueSet>& ) const;
    void			setRandomPosData( int attrib,
					    const ObjectSet<BinIDValueSet>*);
    void			setCubeSampling(CubeSampling);

    bool			setDataPackID(int attrib,DataPack::ID);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
    				{ return DataPackMgr::FlatID; }
    bool			setDataVolume(int attrib,
	    				      const Attrib::DataCubes*);
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
   
    visBase::GridLines*		gridlines()		{ return gridlines_; }

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
	    					BufferString& val,
	    					BufferString& info) const;
    void			getObjectInfo(BufferString&) const;

    virtual void		fillPar(IOPar&, TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    virtual float		calcDist(const Coord3&) const;
    virtual float		maxDist() const;
    virtual Coord3		getNormal(const Coord3&) const;
    virtual bool		allowPicks() const		{ return true; }

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;

    void			setTranslationDragKeys(bool depth, int );
    				/*!<\param depth specifies wheter the depth or
						 the plane setting should be
						 changed.
				    \param keys   combination of OD::ButtonState
				    \note only shift/ctrl/alt are used. */

    int				getTranslationDragKeys(bool depth) const;
    				/*!<\param depth specifies wheter the depth or
						 the plane setting should be
						 returned.
				    \returns	combination of OD::ButtonState*/
    bool                	isVerticalPlane() const;

    static const char*		sKeyDepthKey()		{ return "DepthKey"; }
    static const char*		sKeyPlaneKey()		{ return "PlaneKey"; }

protected:
				~PlaneDataDisplay();
    void			updateMainSwitch();

    void			setData(int attrib,const Attrib::DataCubes*);
    void			setData(int attrib,
	    				const ObjectSet<BinIDValueSet>*);
    void			updateRanges(bool resetpos=false);
    void			updateRanges(bool resetinlcrl=false,
	    				     bool resetz=false);
    void			manipChanged(CallBacker*);
    void			coltabChanged(CallBacker*);
    void			draggerMotion(CallBacker*);
    void			draggerFinish(CallBacker*);
    void			draggerRightClick(CallBacker*);
    void			setDraggerPos(const CubeSampling&);
    void			dataTransformCB(CallBacker*);
    void			setTextureCoords(int sz0,int sz1);
    
    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    void			addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;

    CubeSampling		snapPosition(const CubeSampling&) const;

    visBase::DepthTabPlaneDragger*	dragger_;
    visBase::Material*			draggermaterial_;
    visBase::PickStyle*			rectanglepickstyle_;
    visBase::FaceSet*			rectangle_;
    visBase::GridLines*			gridlines_;
    Orientation				orientation_;
    visBase::FaceSet*			draggerrect_;
    visBase::DrawStyle*			draggerdrawstyle_;

    TypeSet<DataPack::ID>		datapackids_;
    ObjectSet<const Attrib::DataCubes>	volumecache_;
    ObjectSet<BinIDValueSet>		rposcache_;

    CubeSampling			csfromsession_;
    BinID				curicstep_;
    float				curzstep_;
    ZAxisTransform*			datatransform_;
    ZAxisTransformer*			datatransformer_;
    Notifier<PlaneDataDisplay>		moving_;
    Notifier<PlaneDataDisplay>		movefinished_;

    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyTextureRect() { return "Texture rectangle";}
    static const char*		sKeyResolution()  { return "Resolution"; }
    static const char*		sKeyGridLinesID() { return "GridLines ID"; }
};

} // namespace visSurvey


#endif
