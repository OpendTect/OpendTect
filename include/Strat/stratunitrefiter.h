#ifndef stratunitrefiter_h
#define stratunitrefiter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2003 / Sep 2010
 RCS:		$Id: stratunitrefiter.h,v 1.1 2010-09-27 11:05:19 cvsbruno Exp $
________________________________________________________________________


-*/

#include "stratunitref.h"


namespace Strat
{

/*!\brief Iterator on Ref Nodes.
  
 When constructed, returns unit itself (regardless of Pol). First next()
 goes to first (valid) unit.

*/

mClass UnitRefIter
{
public:

    enum Pol		{ All, Leaves, AllNodes, LeavedNodes, NodesOnly };
    static Pol		polOf(UnitRef::Type);
    static Pol		polOf(const UnitRef*);

			UnitRefIter(const NodeUnitRef&,Pol p=All);

    void		reset();
    bool		next();
    UnitRef*		unit()		{ return gtUnit(); }
    const UnitRef*	unit() const	{ return gtUnit(); }

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
