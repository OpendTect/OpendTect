#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "color.h"
#include "valseriesevent.h"

class SeisTrc;

namespace WellTie
{

class PickData;
class Marker;


mExpClass(WellAttrib) PickSetMgr : public CallBacker
{
public:
				PickSetMgr(PickData&);
				~PickSetMgr();

    Notifier<PickSetMgr>	pickadded;

    bool			lastpicksynth_;
    VSEvent::Type		evtype_;

    void			addPick(float,bool,const SeisTrc* trc=0);
    void			addPick(float,float);
    bool			isPick() const;
    bool			isSynthSeisSameSize() const;
    void			clearAllPicks();
    void			clearLastPicks();
    float			findEvent(const SeisTrc&,float) const;
    void			setPickSetPos(bool issynth,int idx,float z);
    void			sortByPos();

    void			getEventTypes(BufferStringSet&) const;
    void			setEventType(const char*);
    const char*			getEventType() const;
    const TypeSet<Marker>&	synthPickSet() const	{ return synthpickset_;}
    const TypeSet<Marker>&	seisPickSet() const	{ return seispickset_; }

protected :
    void			sortByPos(TypeSet<Marker>&);

    TypeSet<Marker>&		synthpickset_;
    TypeSet<Marker>&		seispickset_;
};

} // namespace WellTie
