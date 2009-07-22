#ifndef statrand_h
#define statrand_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: statrand.h,v 1.5 2009-07-22 16:01:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "odset.h"

namespace Stats
{

mClass RandGen
{
public:

    static void		init(int seed=0);
    			//!< If no seed passed, will generate one if needed
    static double	get();
    			//!< Uniform [0-1]
    static double	getNormal(double expectation,double stdev);
    			//!< Normally distributed
    static int		getIndex(int sz);
    			//!< random index in the range [0,sz>

    template <class T>
    static void		subselect(T*,int sz,int targetsz);
    			//!< Does not preserve order.
    			//!< Afterwards, the 'removed' values occupy
    			//!< the indexes targetsz - maxsz-1
    static void		subselect(OD::Set&,int targetsz);
    			//!< Does not preserve order
    			//!< The removed items will really be erased

private:

    static int		seed_;

};


template <class T>
inline void Stats::RandGen::subselect( T* arr, int sz, int targetsz )
{
    for ( int idx=sz-1; idx>=targetsz; idx-- )
    {
	const int notselidx = getIndex( idx );
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
