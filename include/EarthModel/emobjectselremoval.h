#ifndef emobjectselremoval_h
#define emobjectselremoval_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2009
 RCS:		$Id: emobjectselremoval.h,v 1.1 2009-05-18 09:17:41 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "position.h"
#include "selector.h"
#include "task.h"
#include "thread.h"

template <class T> class Selector;

namespace EM
{

class EMObject;    

mClass EMObjectSelectionRemoval : public ParallelTask
{
public:
    			EMObjectSelectionRemoval(EMObject& emobj,
						 const SectionID& secid,
						 const Selector<Coord3>&,
						 int nrrows,int nrcols,
						 int startrow,int startcol);
			~EMObjectSelectionRemoval();

    od_int64		nrIterations() const;
    od_int64            totalNr() const         { return nrIterations(); }
    bool		doWork(od_int64 start,od_int64 stop,int threadid);

    const TypeSet<EM::SubID> getRemovelList()	{ return removelist_; }

protected:
  
    EMObject&			emobj_;
    const SectionID&		sectionid_;
    const Selector<Coord3>& 	selector_;
    int				startrow_;
    int				nrrows_;
    int				startcol_;
    int				nrcols_;
    Threads::Mutex         	mutex_;
    TypeSet<EM::SubID>     	removelist_;    
};

} // EM


#endif
