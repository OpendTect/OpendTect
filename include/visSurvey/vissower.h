#ifndef vissower_h
#define vissower_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		December 2010
 RCS:		$Id$
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "emposid.h"
#include "keyenum.h"
#include "visobject.h"


class Color;
class Coord;
class HorSampling;

namespace visBase { class PolyLine; class PickStyle; };

namespace visSurvey
{

#define mCtrlLeftButton ( (OD::ButtonState) (OD::LeftButton+OD::ControlButton) )

mClass(visSurvey) Sower : public visBase::VisualObjectImpl
{

public:
			Sower(const visBase::VisualObjectImpl* =0);
			~Sower();

    void		setDisplayTransformation( const mVisTrans* );
    void		setEventCatcher( visBase::EventCatcher* );

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

    bool		activate(const Color&,const visBase::EventInfo&,
	    			 int underlyingobjid=-1,
	    			 const HorSampling* workrange=0);

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
    visBase::PickStyle*			pickstyle_;
    bool				linelost_;
    SowingMode				mode_;
    ObjectSet<visBase::EventInfo>	eventlist_;
    int					underlyingobjid_;
    HorSampling*			workrange_;
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



}; // namespace visSurvey

#endif

