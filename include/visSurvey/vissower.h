#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "emposid.h"
#include "keyenum.h"
#include "visevent.h"
#include "visobject.h"
#include "vispolyline.h"
#include "vistransform.h"


class TrcKeySampling;

namespace visSurvey
{

#define mCtrlLeftButton ( (OD::ButtonState) (OD::LeftButton+OD::ControlButton) )

mExpClass(visSurvey) Sower : public visBase::VisualObjectImpl
{

public:
			Sower(visBase::VisualObjectImpl* =nullptr);

    void		setDisplayTransformation(const mVisTrans*) override;
    void		setEventCatcher(visBase::EventCatcher*);

    enum		SowingMode { Lasering=-2, Erasing=-1, Idle=0,
				     Furrowing, FirstSowing, SequentSowing };

    SowingMode		mode() const	{ return mode_; }

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
				 const VisID& underlyingobjid=VisID::udf(),
				 const TrcKeySampling* workrange=nullptr);
    Notifier<Sower>	sowingend;
    Notifier<Sower>	sowing;

protected:
			~Sower();

    bool		isInWorkRange(const visBase::EventInfo&) const ;
    void		tieToWorkRange(const visBase::EventInfo&);
    void		calibrateEventInfo(visBase::EventInfo&);

    bool		acceptMouse(const visBase::EventInfo&);
    bool		acceptTablet(const visBase::EventInfo&);
    bool		acceptLaser(const visBase::EventInfo&);
    bool		acceptEraser(const visBase::EventInfo&);

    EM::PosID		getMarkerID(const visBase::EventInfo&) const;

    void		reset();

    WeakPtr<visBase::VisualObjectImpl>	editobject_;
    RefMan<visBase::EventCatcher>	eventcatcher_;
    ConstRefMan<mVisTrans>		transformation_;
    RefMan<visBase::PolyLine>		sowingline_;
    bool				linelost_ = false;
    SowingMode				mode_			= Idle;
    ObjectSet<visBase::EventInfo>	eventlist_;
    VisID				underlyingobjid_;
    TrcKeySampling*			workrange_		= nullptr;
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

    bool				singleseeded_ = true;

    EM::PosID				curpid_;
    int					curpidstamp_		= mUdf(int);

    int					furrowstamp_;
};

} // namespace visSurvey
