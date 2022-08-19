#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "geometry.h"
#include "keyenum.h"
#include "visobject.h"


class TrcKeyZSampling;

namespace Pick { class Set; class Location; class SetMgr; }
namespace visBase { class PolyLine; };

namespace visSurvey
{

class RandomTrackDisplay;
class Seis2DDisplay;

#define mCtrlLeftButton ( (OD::ButtonState) (OD::LeftButton+OD::ControlButton) )

mExpClass(visSurvey) SeedPainter : public visBase::VisualObjectImpl
{

public:
			SeedPainter();
			~SeedPainter();

    void		setSet(Pick::Set*);
    void		setSetMgr(Pick::SetMgr*);


    void		setDisplayTransformation(const mVisTrans*) override;
    void		setEventCatcher( visBase::EventCatcher* );

    bool		isActive() const	{ return active_; }
    bool		activate();
    void		deActivate();

    static void		setDensity(int perc);
    static int		density();

    static void		setRadius(int nrsamps);
    static int		radius();

protected:

    void		eventCB(CallBacker*);
    void		reset();

    bool		accept(const visBase::EventInfo&);
    bool		acceptMouse(const visBase::EventInfo&);
    bool		acceptTablet(const visBase::EventInfo&);

    void		drawLine(const visBase::EventInfo&);
    void		drawLineOnRandLine(const RandomTrackDisplay*,
					   const visBase::EventInfo&);
    void		drawLineOn2DLine(const Seis2DDisplay*,
					 const visBase::EventInfo&);

    void		paintSeeds(const visBase::EventInfo& curev,
				   const visBase::EventInfo& prevev);
    void		paintSeedsOnInlCrl(const visBase::EventInfo& curev,
				      const visBase::EventInfo& prevev,
				      const TrcKeyZSampling& tkzs,bool isinl);
    void		paintSeedsOnZSlice(const visBase::EventInfo& curev,
				      const visBase::EventInfo& prevev,
				      const TrcKeyZSampling& tkzs);
    void		paintSeedsOnRandLine(const RandomTrackDisplay*,
					const visBase::EventInfo& curev,
					const visBase::EventInfo& prevev);
    void		paintSeedsOn2DLine(const Seis2DDisplay*,
					const visBase::EventInfo& curev,
					const visBase::EventInfo& prevev);

    void		eraseSeeds(const visBase::EventInfo& curev);
    void		eraseSeedsOnRandLine(const RandomTrackDisplay*,
					     const visBase::EventInfo& curev);
    void		eraseSeedsOn2DLine(const Seis2DDisplay*,
					   const visBase::EventInfo& curev);

    visBase::EventCatcher*		eventcatcher_;
    const mVisTrans*			transformation_;
    visBase::PolyLine*			circle_;
    visBase::EventInfo*			prevev_;

    bool				active_ = false;
    bool				isleftbutpressed_ = false;
    RefMan<Pick::Set>			set_;
    Pick::SetMgr*			picksetmgr_;

    static int				density_;
    static int				radius_;
    static TypeSet<Geom::PointI>	circlecoords_;
    static void				mkCircle();

};

} // namespace visSurvey
