#ifndef randomlinegeom_h
#define randomlinegeom_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinegeom.h,v 1.1 2006-12-14 21:44:35 cvsnanne Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "ranges.h"

namespace Geometry
{

class RandomLine : public CallBacker
{
public:
    				RandomLine();
				~RandomLine()	{}

    int				addNode(const BinID&);
    void			insertNode(int,const BinID&);
    void			setNodePosition(int idx,const BinID&);
    void			removeNode(int);
    void			removeNode(const BinID&);

    int				nodeIndex(const BinID&) const;
    int				nrNodes() const;
    const BinID&		nodePosition(int) const;
    void			getAllNodes(TypeSet<BinID>&) const;

    void			setZRange( const Interval<float>& zrg )
				{ zrange_ = zrg; }
    Interval<float>		zRange() const	{ return zrange_; }

    Notifier<RandomLine>	nodeAdded;
    Notifier<RandomLine>	nodeInserted;
    Notifier<RandomLine>	nodeReoved;
    Notifier<RandomLine>	zrangeChanged;

protected:
    TypeSet<BinID>		nodes_;
    Interval<float>		zrange_;
};

} // namespace

#endif
