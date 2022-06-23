#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "vismultiattribsurvobj.h"

#include "emposid.h"
#include "ranges.h"


namespace visBase
{
    class DrawStyle;
    class GeomIndexedShape;
    class PolyLine3D;
    class Transformation;
    class TriangleStripSet;
};

namespace EM { class PolygonBody; }
namespace MPE { class PolygonBodyEditor; }
namespace Geometry { class ExplPolygonSurface; class ExplPlaneIntersection; }
class MarkerStyle3D;


namespace visSurvey
{
class MPEEditor;

/*!\brief


*/

mExpClass(visSurvey) PolygonBodyDisplay : public visBase::VisualObjectImpl,
			    public SurveyObject
{
public:
				PolygonBodyDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,PolygonBodyDisplay,
				    "PolygonBodyDisplay",
				     toUiString(sFactoryKeyword()));
    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    OD::Color			getColor() const;
    void			setColor(OD::Color);
    bool			hasColor() const	  { return true; }
    bool			allowMaterialEdit() const { return true; }

    const OD::LineStyle*	lineStyle() const;
    void			setLineStyle(const OD::LineStyle&);
    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			display(bool polygons,bool body);
    bool			arePolygonsDisplayed() const;
    bool			isBodyDisplayed() const;
    void			setOnlyAtSectionsDisplay(bool yn);
    bool			displayedOnlyAtSections() const;

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			touchAll(bool,bool updatemarker=false);
    EM::PolygonBody*		getEMPolygonBody() const
    				{ return empolygonsurf_; }
    bool			canRemoveSelection() const	{ return true; }
    bool			removeSelections(TaskRunner*);

    const char*			errMsg() const { return errmsg_.str(); }
    virtual void		setPixelDensity(float);

    void			setMarkerStyle(const MarkerStyle3D&);
    const MarkerStyle3D*	markerStyle() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

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

    bool			isPicking() const;
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
    void			setLineRadius(visBase::GeomIndexedShape*);
    void			reMakeIntersectionSurface();
    void			matChangeCB(CallBacker*);


    visBase::EventCatcher*		eventcatcher_;
    const mVisTrans*			displaytransform_;

    visBase::GeomIndexedShape*		bodydisplay_;
    Geometry::ExplPolygonSurface*	explicitbody_;

    visBase::GeomIndexedShape*		polygondisplay_;
    Geometry::ExplPolygonSurface*	explicitpolygons_;

    visBase::GeomIndexedShape*		intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    visBase::PolyLine3D*		nearestpolygonmarker_;
    int					nearestpolygon_;

    EM::PolygonBody*			empolygonsurf_;
    MPE::PolygonBodyEditor*		polygonsurfeditor_;
    visSurvey::MPEEditor*		viseditor_;

    Coord3				mousepos_;

    bool				showmanipulator_;

    OD::Color				nontexturecol_;

    bool				displaypolygons_;
    visBase::DrawStyle*			drawstyle_;
    visBase::TriangleStripSet*		intsurf_;

public:
    void			displayIntersections(bool yn)
				{ setOnlyAtSectionsDisplay(yn); }
    bool			areIntersectionsDisplayed() const
				{ return displayedOnlyAtSections(); }
};

};


