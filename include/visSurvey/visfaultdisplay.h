#ifndef visfaultdisplay_h
#define visfaultdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaultdisplay.h,v 1.2 2008-03-26 13:53:54 cvsjaap Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"


namespace Geometry
{ class ExplFaultStickSurface; }

namespace visBase
{
    class GeomIndexedShape;
    class Transformation;
};


namespace EM { class Fault; }


namespace visSurvey
{
class MPEEditor;

/*!\brief Used for displaying welltracks, markers and logs


*/

class FaultDisplay : public visBase::VisualObjectImpl,
		     public visSurvey::SurveyObject
{
public:
    static FaultDisplay*	create()
				mCreateDataObj(FaultDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

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

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    virtual			~FaultDisplay();
    void			mouseCB(CallBacker*);

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

    void			showKnotMarkers(bool yn);
    void 			updateKnotMarkers();

    visBase::GeomIndexedShape*		displaysurface_;
    visBase::EventCatcher*		eventcatcher_;

    Geometry::ExplFaultStickSurface*	explicitsurface_;
    EM::Fault*				emfault_;
    visSurvey::MPEEditor*		editor_;
    visBase::Transformation*		displaytransform_;
    ObjectSet<visBase::DataObjectGroup> knotmarkers_;
    
    Coord3				mousedisplaypos_;
    CubeSampling			mouseplanecs_;
    EM::PosID				editpid_;
};

};


#endif
