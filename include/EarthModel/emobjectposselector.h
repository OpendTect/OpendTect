#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		May 2009
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
				const SectionID& secid,
				const ObjectSet<const Selector<Coord3> >&);
			~EMObjectPosSelector();

    const TypeSet<EM::SubID>& getSelected() const	{ return poslist_; }

protected:

    od_int64		nrIterations() const override
			    { return nrcols_*nrrows_; }
    bool		doWork( od_int64, od_int64, int ) override;
    bool		doPrepare(int) override;

    void		processBlock(const RowCol&,const RowCol&);
    void		makeListGrow(const RowCol&,const RowCol&,int selresult);

    void		getBoundingCoords(const RowCol&,const RowCol&,
					  Coord3& up,Coord3& down);


    const ObjectSet<const Selector<Coord3> >&	selectors_;

    const EMObject&		emobj_;
    const SectionID&		sectionid_;
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

