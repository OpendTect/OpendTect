#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emposid.h"
#include "selector.h"
#include "paralleltask.h"
#include "thread.h"

template <class T> class Selector;

namespace EM
{

class EMObject;

/*!
\brief EMObject position selector
*/

mExpClass(EarthModel) EMObjectPosSelector : public ParallelTask
{
public:
			EMObjectPosSelector(const EMObject& emobj,
				const ObjectSet<const Selector<Coord3> >&);
    mDeprecated("Use without SectionID")
			EMObjectPosSelector(const EMObject& emobj,
				const SectionID& secid,
				const ObjectSet<const Selector<Coord3> >& pos)
			    : EMObjectPosSelector(emobj,pos)		{}
			~EMObjectPosSelector();

    const TypeSet<EM::SubID>& getSelected() const	{ return poslist_; }

protected:

    od_int64		nrIterations() const override
			    { return (od_int64)nrcols_ * nrrows_; }
    bool		doWork( od_int64, od_int64, int ) override;
    bool		doPrepare(int) override;

    void		processBlock(const RowCol&,const RowCol&);
    void		makeListGrow(const RowCol&,const RowCol&,int selresult);

    void		getBoundingCoords(const RowCol&,const RowCol&,
					  Coord3& up,Coord3& down);


    const ObjectSet<const Selector<Coord3> >&	selectors_;

    const EMObject&		emobj_;
    const SectionID		sectionid_		= EM::SectionID::def();
    int				startrow_;
    int				nrrows_;
    int				startcol_;
    int				nrcols_;
    const float*		zvals_;

    TypeSet<RowCol>		starts_;
    TypeSet<RowCol>		stops_;
    Threads::ConditionVar	lock_;
    bool			finished_;
    int				nrwaiting_;
    int				nrthreads_;

    TypeSet<EM::SubID>		poslist_;
};

} // namespace EM
