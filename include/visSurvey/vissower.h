#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		December 2010
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "emposid.h"
#include "keyenum.h"
#include "visobject.h"


class TrcKeySampling;

namespace visBase { class PolyLine; };

namespace visSurvey
{

#define mCtrlLeftButton ( (OD::ButtonState) (OD::LeftButton+OD::ControlButton) )

mExpClass(visSurvey) Sower : public visBase::VisualObjectImpl
{

public:
			Sower(const visBase::VisualObjectImpl* =nullptr);
			~Sower();

    void		setDisplayTransformation(const mVisTrans*) override;
    void		setEventCatcher(visBase::EventCatcher*);

    enum		SowingMode { Lasering=-2, Erasing=-1, Idle=0,
				     Furrowing, FirstSowing, SequentSowing };

    SowingMode		mode()				{ return mode_; }

    void		reInitSettings();

    void		reverseSowingOrder(bool yn=true);
    void		alternateSowingOrder(bool yn=true);
    void		intersow(bool yn=true);

    void		setSequentSowMask(bool yn=true,
				      OD::ButtonState mask=OD::LeftButton);
    void		setIfDragInvertMask(bool yn=true,
				      OD::ButtonState mask=OD::ShiftButton);
    void		setLaserMask(bool yn=true,
				      OD::ButtonState mask=OD::LeftButton);
    void		setEraserMask(bool yn=true,
				      OD::ButtonState mask=mCtrlLeftButton);

    bool		moreToSow() const;
    void		stopSowing();

    Coord3		pivotPos() const;

    bool		accept(const visBase::EventInfo&);

    bool		activate(const OD::Color&,const visBase::EventInfo&,
				 VisID underlyingobjid=VisID::udf(),
				 const TrcKeySampling* workrange=0);
    Notifier<Sower>	sowingend;
    Notifier<Sower>	sowing;

protected:

    bool		isInWorkRange(const visBase::EventInfo&) const ;
    void		tieToWorkRange(const visBase::EventInfo&);
    void		calibrateEventInfo(visBase::EventInfo&);

    bool		acceptMouse(const visBase::EventInfo&);
    bool		acceptTablet(const visBase::EventInfo&);
    bool		acceptLaser(const visBase::EventInfo&);
    bool		acceptEraser(const visBase::EventInfo&);

    EM::PosID		getMarkerID(const visBase::EventInfo&) const;

    void		reset();

    const visBase::VisualObjectImpl*	editobject_;
    visBase::EventCatcher*		eventcatcher_;
    const mVisTrans*			transformation_;
    visBase::PolyLine*			sowingline_;
    bool				linelost_;
    SowingMode				mode_;
    ObjectSet<visBase::EventInfo>	eventlist_;
    VisID				underlyingobjid_;
    TrcKeySampling*			workrange_;
    TypeSet<Coord>			mousecoords_;
    TypeSet<int>			bendpoints_;

    bool				reversesowingorder_;
    bool				alternatesowingorder_;
    bool				intersow_;

    OD::ButtonState			pressedbutstate_;
    OD::ButtonState			sequentsowmask_;
    OD::ButtonState			ifdraginvertmask_;
    OD::ButtonState			lasermask_;
    OD::ButtonState			erasermask_;

    bool				singleseeded_;

    EM::PosID				curpid_;
    int					curpidstamp_;

    int					furrowstamp_;
};



} // namespace visSurvey

