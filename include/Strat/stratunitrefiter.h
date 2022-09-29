#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"
#include "stratunitref.h"


namespace Strat
{

/*!\brief Iterator on Ref Nodes.

 When constructed, returns unit itself (regardless of Pol). First next()
 goes to first (valid) unit.

*/

mExpClass(Strat) UnitRefIter
{
public:

    enum Pol		{ All, Leaves, AllNodes, LeavedNodes, NodesOnly };
    static Pol		polOf(UnitRef::Type);
    static Pol		polOf(const UnitRef*);

			UnitRefIter(const NodeUnitRef&,Pol p=All);
			UnitRefIter(const UnitRefIter&);
			~UnitRefIter();
    UnitRefIter&	operator =(const UnitRefIter&);

    void		reset();
    bool		next();
    UnitRef*		unit()		{ return gtUnit(); }
    const UnitRef*	unit() const	{ return gtUnit(); }
    Pol			pol() const	{ return pol_; }
    void		setPol( Pol p )	{ pol_ = p; reset(); }
    Interval<int>	levelRange() const;

    static bool		isValid(const UnitRef&,Pol);

protected:

    Pol			pol_;
    NodeUnitRef*	itnode_;
    NodeUnitRef*	curnode_;
    int			curidx_;

    UnitRef*		gtUnit() const;
    bool		toNext();

};

} // namespace Strat
