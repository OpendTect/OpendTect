#ifndef statrand_h
#define statrand_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include "odset.h"

namespace Stats
{

mClass(Algo) RandGen
{
public:

			RandGen();

    void		init(int seed=0);
    			//!< If no seed passed, will generate one if needed
    double		get();
    			//!< Uniform [0-1]
    int			getInt();
    			//!< Uniform int
    double		getNormal(double expectation,double stdev);
    			//!< Normally distributed
    int	    		getIndex(int sz);
    			//!< random index in the range [0,sz>
    int			getIndexFast(int sz,int seed);
    			//!< getIndex using a very simple random generator
    od_int64		getIndex(od_int64 sz);
    			//!< random index in the range [0,sz>
    od_int64		getIndexFast(od_int64 sz,od_int64 seed);
    			//!< getIndex using a very simple random generator

    template <class T,class SzTp>
    void		subselect(T*,SzTp sz,SzTp targetsz);
    			//!< Does not preserve order.
    			//!< Afterwards, the 'removed' values occupy
    			//!< the indexes targetsz - maxsz-1
    void		subselect(OD::Set&,int targetsz);
    			//!< Does not preserve order
    			//!< The removed items will really be erased

private:

    int			seed_;

};


mGlobal(Algo) RandGen randGen();


template <class T,class SzTp>
inline void Stats::RandGen::subselect( T* arr, SzTp sz, SzTp targetsz )
{
    for ( SzTp idx=sz-1; idx>=targetsz; idx-- )
    {
	const SzTp notselidx = getIndex( idx );
	if ( notselidx != idx )
	    Swap( arr[notselidx], arr[idx] );
    }
}


inline void Stats::RandGen::subselect( OD::Set& ods, int targetsz )
{
    const int sz = ods.nrItems();
    if ( sz <= targetsz ) return;

    for ( int idx=sz-1; idx>=targetsz; idx-- )
    {
	const int notselidx = getIndex( idx );
	if ( notselidx != idx )
	    ods.swap( notselidx, idx );
    }

    ods.remove( targetsz, sz-1 );
}

}; // namespace Stats

#endif

