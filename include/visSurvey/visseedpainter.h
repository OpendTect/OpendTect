#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		December 2020
 RCS:		$Id$
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "geometry.h"
#include "keyenum.h"
#include "visobject.h"


class TrcKeySampling;

namespace Pick { class Set; class Location; class SetMgr; }
namespace visBase { class PolyLine; };

namespace visSurvey
{

#define mCtrlLeftButton ( (OD::ButtonState) (OD::LeftButton+OD::ControlButton) )

mExpClass(visSurvey) SeedPainter : public visBase::VisualObjectImpl
{

public:
			SeedPainter();
			~SeedPainter();

    void		setSet(Pick::Set*);
    void		setSetMgr(Pick::SetMgr*);

    void		setDisplayTransformation( const mVisTrans* );
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
    bool		accept(const visBase::EventInfo&);
    bool		acceptMouse(const visBase::EventInfo&);
    bool		acceptTablet(const visBase::EventInfo&);

    void		reset();
    void		drawLine(const visBase::EventInfo&);
    void		paintSeeds(const visBase::EventInfo& curev,
	    			   const visBase::EventInfo& prevev);
    void		eraseSeeds(const visBase::EventInfo& curev,
	    			   const visBase::EventInfo& prevev);

    visBase::EventCatcher*		eventcatcher_;
    const mVisTrans*			transformation_;
    visBase::PolyLine*			circle_;
    visBase::EventInfo*			prevev_;

    bool				active_ = false;
    Pick::Set*				set_;
    Pick::SetMgr*			picksetmgr_;

    static int				density_;
    static int				radius_;
    static TypeSet<Geom::PointI>	circlecoords_;
    static void				mkCircle();

};

}; // namespace visSurvey

