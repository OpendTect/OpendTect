#ifndef stratseisevent_h
#define stratseisevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
 RCS:           $Id: stratseisevent.h,v 1.3 2012-08-03 13:00:46 cvskris Exp $
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "valseriesevent.h"
#include "ranges.h"
class SeisTrc;


namespace Strat
{
class Level;

/*!\brief Event tied to a stratigraphic level. */

mClass(WellAttrib) SeisEvent
{
public:
    			SeisEvent(const Level* l=0,
				   VSEvent::Type t=VSEvent::None);

    float		snappedTime(const SeisTrc&) const;
    bool		snapPick(SeisTrc&) const;

    const Strat::Level*	level_;
    VSEvent::Type	evtype_;
    float		offs_;
    StepInterval<float>	extrwin_;

};

} // namespace

#endif

