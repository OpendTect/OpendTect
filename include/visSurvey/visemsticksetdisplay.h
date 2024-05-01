#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "draw.h"
#include "emfault.h"
#include "viscoord.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "zaxistransform.h"

namespace EM
{
    class FaultStickSet;
}

namespace visBase
{
    class MarkerSet;
    class Transformation;
    class PolygonSelection;
}

namespace Geometry { class FaultStickSet; }
namespace Survey { class Geometry3D; }

namespace visSurvey
{

mExpClass(visSurvey) StickSetDisplay
{
public:

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			polygonSelectionCB();
    void			updateStickMarkerSet();
    void			getMousePosInfo(const visBase::EventInfo&,
					       Coord3& xyzpos,BufferString& val,
					       uiString& info) const;
    const MarkerStyle3D*	markerStyle() const;
    void			setMarkerStyle(const MarkerStyle3D&);
    void			setStickMarkerStyle(const MarkerStyle3D&);

    bool			areAllKnotsHidden() const;
    virtual void		hideAllKnots(bool);
    bool			isAlreadyTransformed() const;


protected:
				StickSetDisplay(bool isfss);
    virtual			~StickSetDisplay();

    Geometry::FaultStickSet*	faultStickSetGeometry();
    void			stickSelectionCB(CallBacker*,
						 const Survey::Geometry3D*);
    void			setCurScene(Scene*);
    ConstRefMan<Scene>		getCurScene() const;
    RefMan<Scene>		getCurScene();
    bool			matchMarker(int,const Coord3,const Coord3,
					    const Coord3);

    RefMan<EM::Fault>		fault_;
    ConstRefMan<mVisTrans>	displaytransform_;
    bool			faultstickset_;
    bool			ctrldown_		= false;
    bool			showmanipulator_	= false;
    bool			stickselectmode_	= false;
    bool			displaysticks_		= false;
    bool			hideallknots_		= true;
    bool			pickmarker_		= false;
    WeakPtr<Scene>		ownerscene_;
    RefMan<visBase::EventCatcher> eventcatcher_;
    RefMan<ZAxisTransform>	zaxistransform_;

    RefObjectSet<visBase::MarkerSet> knotmarkersets_;
    struct StickIntersectPoint
    {
	Coord3			pos_;
	int			sticknr_;
    };
    ObjectSet<StickIntersectPoint>  stickintersectpoints_;

};

} // namespace visSurvey
