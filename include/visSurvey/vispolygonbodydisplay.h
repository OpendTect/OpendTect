#ifndef vispolygonbodydisplay_h
#define vispolygonbodydisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispolygonbodydisplay.h,v 1.9 2011-04-28 07:00:12 cvsbert Exp $
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
    class PickStyle;
    class ShapeHints;
    class IndexedPolyLine3D;
};

namespace EM { class PolygonBody; }
namespace MPE { class PolygonBodyEditor; }
namespace Geometry { class ExplPolygonSurface; class ExplPlaneIntersection; }


namespace visSurvey
{
class MPEEditor;

/*!\brief 


*/

mClass PolygonBodyDisplay : public visBase::VisualObjectImpl,
			    public SurveyObject
{
public:
    static PolygonBodyDisplay*	create()
				mCreateDataObj(PolygonBodyDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    Color			getColor() const;
    void			setColor(Color);
    bool			hasColor() const	  { return true; }
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void			setRightHandSystem(bool);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			display(bool polygons,bool body);
    bool			arePolygonsDisplayed() const;
    bool			isBodyDisplayed() const;
    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			touchAll(bool,bool updatemarker=false);
    EM::PolygonBody*		getEMPolygonBody() const 
    				{ return empolygonsurf_; }
    bool			canRemoveSelecion() const	{ return true; }
    void			removeSelection(const Selector<Coord3>&,
						TaskRunner*);

    const char*			errMsg() const { return errmsg_.str(); }

protected:

    virtual			~PolygonBodyDisplay();
    void			otherObjectsMoved(
	    			    const ObjectSet<const SurveyObject>&,
				    int whichobj);

    void			updatePolygonDisplay();
    void			updateSingleColor();
    void			updateManipulator();

    static const char*		sKeyEMPolygonSurfID()	
    				{ return "EMPolygonsurface ID"; }

    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);

    void 			updateNearestPolygonMarker();
    void			setNewIntersectingPolygon(const Coord3& normal,
	    						  const Coord3& pt);
       				/*<Given a plane3(nomal, pt), calculate the 
				   intersections of known polygons with the 
				   intersection line between plane3 and polygon
				   plane. */

    Coord3			disp2world(const Coord3& displaypos) const;

    visBase::EventCatcher*		eventcatcher_;
    visBase::Transformation*		displaytransform_;
    visBase::ShapeHints*		shapehints_;

    visBase::GeomIndexedShape*		bodydisplay_;
    Geometry::ExplPolygonSurface*	explicitbody_;

    visBase::GeomIndexedShape*		polygondisplay_;
    Geometry::ExplPolygonSurface*	explicitpolygons_;

    visBase::GeomIndexedShape*		intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    visBase::PickStyle*			nearestpolygonmarkerpickstyle_;
    visBase::IndexedPolyLine3D*		nearestpolygonmarker_;
    int					nearestpolygon_;

    EM::PolygonBody*			empolygonsurf_;
    MPE::PolygonBodyEditor*		polygonsurfeditor_;
    visSurvey::MPEEditor*		viseditor_;

    Coord3				mousepos_;

    bool				showmanipulator_;

    Color				nontexturecol_;

    bool				displaypolygons_;
};

};


#endif
