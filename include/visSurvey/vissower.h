#ifndef vissower_h
#define vissower_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		December 2010
 RCS:		$Id: vissower.h,v 1.2 2011-01-27 14:56:39 cvsjaap Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "keyenum.h"
#include "visobject.h"


class Color;
class Coord;

namespace visBase { class PolyLine; };

namespace visSurvey
{

#define mCtrlLeftButton ( (OD::ButtonState) (OD::LeftButton+OD::ControlButton) )

mClass Sower : public visBase::VisualObjectImpl
{

public:
			Sower(const visBase::VisualObjectImpl&);
			~Sower();

    void		setDisplayTransformation( mVisTrans* );
    void		setEventCatcher( visBase::EventCatcher* );

    enum		SowingMode { Lasering=-2, Erasing=-1, Idle=0,
				     Furrowing, FirstSowing, SequentSowing };

    SowingMode		mode()				{ return mode_; }

    void		reverseSowingOrder(bool yn=true);
    void		alternateSowingOrder(bool yn=true);

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
    bool		activate(const Color&,const visBase::EventInfo&);

protected:
    bool		acceptMouse(const visBase::EventInfo&);
    bool		acceptTablet(const visBase::EventInfo&);
    bool		acceptLaser(const visBase::EventInfo&);
    bool		acceptEraser(const visBase::EventInfo&);

    EM::PosID		getMarkerID(const visBase::EventInfo&) const; 

    void		reset();

    const visBase::VisualObjectImpl&	editobject_;
    visBase::EventCatcher*		eventcatcher_;
    visBase::PolyLine*			sowingline_;
    bool				linelost_;
    SowingMode				mode_;
    ObjectSet<visBase::EventInfo>	eventlist_;
    TypeSet<Coord>			mousecoords_;
    TypeSet<int>			bendpoints_;

    bool				reversesowingorder_;
    bool				alternatesowingorder_;

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
