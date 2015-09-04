#ifndef emobjectposselector_h
#define emobjectposselector_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		May 2009
 RCS:		$Id$
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
						const Selector<Coord3>&,
						int nrrows,int nrcols,
						int startrow,int startcol);
			~EMObjectPosSelector();

    const TypeSet<EM::SubID>& getSelected() const	{ return removelist_; }

protected:

    od_int64		nrIterations() const	{ return nrcols_*nrrows_; }
    bool		doWork( od_int64, od_int64, int );
    bool		doPrepare(int);

    void		processBlock(const RowCol&,const RowCol&);
    void		makeListGrow(const RowCol&,const RowCol&,int selresult);

    void		getBoundingCoords(const RowCol&,const RowCol&,
					  Coord3& up,Coord3& down);


    const EMObject&		emobj_;
    const SectionID&		sectionid_;
    const Selector<Coord3>&	selector_;
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

    TypeSet<EM::SubID>	removelist_;
};

} // namespace EM

#endif

