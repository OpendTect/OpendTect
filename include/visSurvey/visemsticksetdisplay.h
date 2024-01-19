#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "viscoord.h"
#include "draw.h"

namespace EM
{
    class Fault;
    class FaultStickSet;
}

namespace visBase
{
    class MarkerSet;
    class EventCatcher;
    class Transformation;
    class PolygonSelection;
}

namespace Geometry { class FaultStickSet; }
namespace Survey { class Geometry3D; }

class ZAxisTransform;

namespace visSurvey
{
class Scene;

mExpClass(visSurvey) StickSetDisplay
{
public:
				StickSetDisplay(bool);
				~StickSetDisplay();
    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			polygonSelectionCB();
    void			updateStickMarkerSet();
    void			getMousePosInfo(const visBase::EventInfo&,
					       Coord3& xyzpos,BufferString& val,
					       BufferString& info) const;
    const MarkerStyle3D*	markerStyle() const;
    void			setMarkerStyle(const MarkerStyle3D&);
    void			setStickMarkerStyle(const MarkerStyle3D&);

    bool			areAllKnotsHidden() const;
    virtual void		hideAllKnots(bool);
    bool			isAlreadyTransformed() const;

protected:
    Geometry::FaultStickSet*	faultStickSetGeometry();
    void			stickSelectionCB(CallBacker*,
						 const Survey::Geometry3D*);
    void			setCurScene( visSurvey::Scene* scene )
					   { ownerscene_ = scene; }
    const visSurvey::Scene*	getCurScene() { return ownerscene_; }
    bool			matchMarker(int,const Coord3,const Coord3,
					    const Coord3);
    EM::Fault*			fault_;
    const mVisTrans*		displaytransform_;
    bool			ctrldown_;
    bool			showmanipulator_;
    bool			stickselectmode_;
    bool			displaysticks_;
    bool			hideallknots_;
    bool			faultstickset_;
    bool			pickmarker_;
    visSurvey::Scene*		ownerscene_;
    visBase::EventCatcher*	eventcatcher_;
    ZAxisTransform*		zaxistransform_		= nullptr;

    ObjectSet<visBase::MarkerSet>   knotmarkersets_;
    struct StickIntersectPoint
    {
	Coord3			pos_;
	int			sticknr_;
    };
    ObjectSet<StickIntersectPoint>  stickintersectpoints_;

};

} // namespace visSurvey
