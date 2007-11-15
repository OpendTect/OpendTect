#ifndef randomlinegeom_h
#define randomlinegeom_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinegeom.h,v 1.5 2007-11-15 16:52:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "ranges.h"

class IOPar;

namespace Geometry
{

class RandomLineSet;

class RandomLine : public CallBacker
{
public:
    			RandomLine();
			~RandomLine()		{}
    bool		isEmpty() const		{ return nodes_.isEmpty(); }

    int			addNode(const BinID&);
    void		insertNode(int,const BinID&);
    void		setNodePosition(int idx,const BinID&);
    void		removeNode(int);
    void		removeNode(const BinID&);

    int			nodeIndex(const BinID&) const;
    int			nrNodes() const;
    const BinID&	nodePosition(int) const;
    void		allNodePositions(TypeSet<BinID>&) const;

    void		setZRange( const Interval<float>& rg )
    			{ zrange_ = rg; zrangeChanged.trigger(); }
    Interval<float>	zRange() const		{ return zrange_; }

    Notifier<RandomLine> nodeAdded;
    Notifier<RandomLine> nodeInserted;
    Notifier<RandomLine> nodeRemoved;
    Notifier<RandomLine> zrangeChanged;

    RandomLineSet*	lineSet()		{ return lset_; }
    const RandomLineSet* lineSet() const	{ return lset_; }

protected:

    TypeSet<BinID>	nodes_;
    Interval<float>	zrange_;
    RandomLineSet*	lset_;

    friend class	RandomLineSet;
};


class RandomLineSet
{
public:

    			RandomLineSet();
    virtual		~RandomLineSet();
    bool		isEmpty() const		{ return lines_.isEmpty(); }

    int			size() const		{ return lines_.size(); }
    const ObjectSet<RandomLine>& lines() const	{ return lines_; }
    void		removeLine( int idx )	{ delete lines_.remove(idx); }
    void		addLine( RandomLine* rl )
    			{ rl->lset_ = this; lines_ += rl; }

    const IOPar&	pars() const		{ return pars_; }
    IOPar&		pars()			{ return pars_; }

protected:

    ObjectSet<RandomLine>	lines_;
    IOPar&			pars_;

};


} // namespace

#endif
