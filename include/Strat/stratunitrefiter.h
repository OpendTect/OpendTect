#ifndef stratunitrefiter_h
#define stratunitrefiter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2003 / Sep 2010
 RCS:		$Id: stratunitrefiter.h,v 1.5 2012-08-03 13:00:43 cvskris Exp $
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

mClass(Strat) UnitRefIter
{
public:

    enum Pol		{ All, Leaves, AllNodes, LeavedNodes, NodesOnly };
    static Pol		polOf(UnitRef::Type);
    static Pol		polOf(const UnitRef*);

			UnitRefIter(const NodeUnitRef&,Pol p=All);
			UnitRefIter( const UnitRefIter& uri )	{ *this = uri; }
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

} // namespace

#endif

