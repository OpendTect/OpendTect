#ifndef visfaultdisplay_h
#define visfaultdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaultdisplay.h,v 1.4 2008-05-15 20:28:25 cvskris Exp $
________________________________________________________________________


-*/

#include "vismultiattribsurvobj.h"

#include "emposid.h"
#include "ranges.h"


class DataPointSet;

namespace visBase
{
    class GeomIndexedShape;
    class Transformation;
    class ShapeHints;
};

namespace EM { class Fault; }
namespace MPE { class FaultEditor; }
namespace Geometry { class ExplFaultStickSurface; }


namespace visSurvey
{
class MPEEditor;

/*!\brief Used for displaying welltracks, markers and logs


*/

class FaultDisplay : public MultiTextureSurveyObject
{
public:
    static FaultDisplay*	create()
				mCreateDataObj(FaultDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    virtual int			nrResolutions() const;
    virtual void		setResolution(int);

    SurveyObject::AttribFormat	getAttributeFormat() const
				{ return SurveyObject::RandomPos; }
    void			getRandomPos(DataPointSet&) const	{}
    void			setRandomPosData(int,const DataPointSet*) {}

    bool			hasColor() const		{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const	{ return true; }
    NotifierAccess*		materialChange();

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void			setRightHandSystem(bool);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			display(bool sticks,bool panels);
    bool			areSticksDisplayed() const;
    bool			arePanelsDisplayed() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

protected:

    virtual			~FaultDisplay();

    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    virtual void		addCache();
    virtual void		removeCache(int);
    virtual void		swapCache(int,int);
    virtual void		emptyCache(int);
    virtual bool		hasCache(int) const;

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);

    bool			segmentInPlane(const EM::PosID& knot1,
	                                       const EM::PosID& knot2,
					       const CubeSampling* plane) const;

    float			nearestStickSegment(const Coord3& displaypos,
	    				Coord3& nearestpos,
					EM::PosID& knot1,EM::PosID& knot2,
					const CubeSampling* plane=0,
					const bool* verticaldir=0,
					const EM::PosID* stickpid=0) const;

    void			getNearestKnots(TypeSet<EM::PosID>&) const;

    EM::PosID			getMarkerPid( const Coord3& markerpos );
    void			setEditID(const EM::PosID&);

    void 			updateKnotMarkers();
    void			updateKnotMarkerColor( const Coord3& mousepos );

    visBase::GeomIndexedShape*		displaysurface_;
    visBase::EventCatcher*		eventcatcher_;

    Geometry::ExplFaultStickSurface*	explicitsurface_;
    EM::Fault*				emfault_;
    visSurvey::MPEEditor*		viseditor_;
    MPE::FaultEditor*			faulteditor_;
    visBase::ShapeHints*		shapehints_;
    visBase::Transformation*		displaytransform_;

    Coord3				mousepos_;
};

};


#endif
