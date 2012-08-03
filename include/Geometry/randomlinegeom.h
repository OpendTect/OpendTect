#ifndef randomlinegeom_h
#define randomlinegeom_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinegeom.h,v 1.12 2012-08-03 13:00:28 cvskris Exp $
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "position.h"
#include "ranges.h"
#include "namedobj.h"

class CubeSampling;
class IOPar;
class Line2;

namespace Geometry
{

class RandomLineSet;

mClass(Geometry) RandomLine : public NamedObject
{
public:
    			RandomLine(const char* nm=0);
			~RandomLine()		{}
    bool		isEmpty() const		{ return nodes_.isEmpty(); }

    int			addNode(const BinID&);
    void		insertNode(int,const BinID&);
    void		setNodePosition(int idx,const BinID&);
    void		removeNode(int);
    void		removeNode(const BinID&);
    void		limitTo(const CubeSampling&); // nrNodes should be 2

    int			nodeIndex(const BinID&) const;
    int			nrNodes() const;
    const BinID&	nodePosition(int) const;
    void		allNodePositions(TypeSet<BinID>&) const;
    static void		getPathBids(const TypeSet<BinID>& knots,
	    				  TypeSet<BinID>& path,
					  bool allowduplicate=false,
					  TypeSet<int>* segments=0); 

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


mClass(Geometry) RandomLineSet
{
public:

    			RandomLineSet();
			RandomLineSet(const RandomLine&,double dist,
				      bool parallel);
			//!< dist in XY units

    virtual		~RandomLineSet();
    bool		isEmpty() const		{ return lines_.isEmpty(); }

    int			size() const		{ return lines_.size(); }
    const ObjectSet<RandomLine>& lines() const	{ return lines_; }
    void		removeLine( int idx )	{ delete lines_.remove(idx); }
    void		addLine( RandomLine* rl )
    			{ rl->lset_ = this; lines_ += rl; }
    void		limitTo(const CubeSampling&);

    const IOPar&	pars() const		{ return pars_; }
    IOPar&		pars()			{ return pars_; }

protected:

    ObjectSet<RandomLine>	lines_;
    IOPar&			pars_;

    void		createParallelLines(const Line2& baseline,double dist);
};


} // namespace

#endif

