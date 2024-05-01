#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "emposid.h"
#include "empolygonbody.h"
#include "mpeengine.h"
#include "polygonsurfeditor.h"
#include "ranges.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismpeeditor.h"
#include "vismultiattribsurvobj.h"
#include "visobject.h"
#include "vispolyline.h"
#include "vistransform.h"
#include "vistristripset.h"


class MarkerStyle3D;
namespace Geometry { class ExplPolygonSurface; class ExplPlaneIntersection; }


namespace visSurvey
{

/*!\brief
*/

mExpClass(visSurvey) PolygonBodyDisplay : public visBase::VisualObjectImpl
					, public SurveyObject
{
public:
				PolygonBodyDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, PolygonBodyDisplay,
				    "PolygonBodyDisplay",
				    ::toUiString(sFactoryKeyword()) )

    MultiID			getMultiID() const override;
    bool			isInlCrl() const override { return false; }

    OD::Color			getColor() const override;
    void			setColor(OD::Color) override;
    bool			hasColor() const override	{ return true; }
    bool			allowMaterialEdit() const override
				{ return true; }

    const OD::LineStyle*	lineStyle() const override;
    void			setLineStyle(const OD::LineStyle&) override;
    void			showManipulator(bool) override;
    bool			isManipulatorShown() const override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;

    void			display(bool polygons,bool body);
    bool			arePolygonsDisplayed() const;
    bool			isBodyDisplayed() const;
    void			setOnlyAtSectionsDisplay(bool yn) override;
    bool			displayedOnlyAtSections() const override;

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			touchAll(bool,bool updatemarker=false);
    const EM::PolygonBody*	getEMPolygonBody() const
				{ return empolygonsurf_.ptr(); }
    EM::PolygonBody*		getEMPolygonBody()
				{ return empolygonsurf_.ptr(); }
    bool			canRemoveSelection() const override
				{ return true; }
    bool			removeSelections(TaskRunner*) override;

    const char*			errMsg() const override { return errmsg_.str();}
    void			setPixelDensity(float) override;

    void			setMarkerStyle(const MarkerStyle3D&) override;
    const MarkerStyle3D*	markerStyle() const override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:

    virtual			~PolygonBodyDisplay();

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    const VisID& whichobj) override;

    void			updatePolygonDisplay();
    void			updateSingleColor();
    void			updateManipulator();

    static const char*		sKeyEMPolygonSurfID()
				{ return "EMPolygonsurface ID"; }

    bool			isPicking() const override;
    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);

    void			updateNearestPolygonMarker();
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


    RefMan<visBase::EventCatcher>	eventcatcher_;
    ConstRefMan<mVisTrans>		displaytransform_;

    RefMan<visBase::GeomIndexedShape>	bodydisplay_;
    Geometry::ExplPolygonSurface*	explicitbody_		= nullptr;

    RefMan<visBase::GeomIndexedShape>	polygondisplay_;
    Geometry::ExplPolygonSurface*	explicitpolygons_	= nullptr;

    RefMan<visBase::GeomIndexedShape>	intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_	= nullptr;
    WeakPtrSet<visBase::VisualObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    RefMan<visBase::PolyLine3D>		nearestpolygonmarker_;
    int					nearestpolygon_		= mUdf(int);

    RefMan<EM::PolygonBody>		empolygonsurf_;
    RefMan<MPE::PolygonBodyEditor>	polygonsurfeditor_;
    RefMan<MPEEditor>			viseditor_;

    Coord3				mousepos_;

    bool				showmanipulator_	= false;

    OD::Color				nontexturecol_;

    bool				displaypolygons_	= false;
    RefMan<visBase::DrawStyle>		drawstyle_;
    RefMan<visBase::TriangleStripSet>	intsurf_;

public:
    void			displayIntersections(bool yn)
				{ setOnlyAtSectionsDisplay(yn); }
    bool			areIntersectionsDisplayed() const
				{ return displayedOnlyAtSections(); }
};

};
