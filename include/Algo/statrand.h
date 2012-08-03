#ifndef statrand_h
#define statrand_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: statrand.h,v 1.9 2012-08-03 13:00:06 cvskris Exp $
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

    static void		init(int seed=0);
    			//!< If no seed passed, will generate one if needed
    static double	get();
    			//!< Uniform [0-1]
    static int		getInt();
    			//!< Uniform int
    static double	getNormal(double expectation,double stdev);
    			//!< Normally distributed
    static int		getIndex(int sz);
    			//!< random index in the range [0,sz>
    static int		getIndexFast(int sz,int seed);
    			//!< getIndex using a very simple random generator
    static od_int64	getIndex(od_int64 sz);
    			//!< random index in the range [0,sz>
    static od_int64	getIndexFast(od_int64 sz,od_int64 seed);
    			//!< getIndex using a very simple random generator

    template <class T,class SzTp>
    static void		subselect(T*,SzTp sz,SzTp targetsz);
    			//!< Does not preserve order.
    			//!< Afterwards, the 'removed' values occupy
    			//!< the indexes targetsz - maxsz-1
    static void		subselect(OD::Set&,int targetsz);
    			//!< Does not preserve order
    			//!< The removed items will really be erased

private:

    static int		seed_;

};


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

