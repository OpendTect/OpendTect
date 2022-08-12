#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
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
    void	    		getMousePosInfo(const visBase::EventInfo&,
					       Coord3& xyzpos,BufferString& val,
					       BufferString& info) const;
    const MarkerStyle3D*	markerStyle() const;
    void			setMarkerStyle(const MarkerStyle3D&);
    void			setStickMarkerStyle(const MarkerStyle3D&);

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

    ObjectSet<visBase::MarkerSet>   knotmarkersets_;
    struct StickIntersectPoint
    {
	Coord3			pos_;
	int			sticknr_;
    };
    ObjectSet<StickIntersectPoint>  stickintersectpoints_;

};

} // namespace visSurvey
