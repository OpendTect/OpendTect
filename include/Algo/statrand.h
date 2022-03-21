#pragma once
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

/*!\brief Random Generator */

mExpClass(Algo) RandomGenerator
{
public:

    virtual		~RandomGenerator()	{}
    virtual void	init(int seed);
				// seed != 0 gives repeatable numbers

    virtual double	get() const		= 0;	// returns in [0,1]

};


/*!\brief Uniform Random Generator */

mExpClass(Algo) RandGen : public RandomGenerator
{
public:

			RandGen();

    virtual double	get() const;
			//!< Uniform [0-1]
    int			getInt() const;
			//!< Uniform int
    int			getInt(int min, int max) const;
			//!< random int in the range [min,max]>
    int			getIndex(int sz) const;
			//!< random index in the range [0,sz>
    mDeprecated("Use getIndex")
    int			getIndexFast(int sz,int seed) const;
			//!< getIndex using a very simple random generator
    od_int64		getIndex(od_int64 sz) const;
			//!< random index in the range [0,sz>
    mDeprecated("Use getIndex")
    od_int64		getIndexFast(od_int64 sz,od_int64 seed) const;
			//!< getIndex using a very simple random generator

    template <class T,class SzTp>
    void		subselect(T*,SzTp sz,SzTp targetsz) const;
			//!< Does not preserve order.
			//!< Afterwards, the 'removed' values occupy
			//!< the indexes targetsz - maxsz-1
    template <class T>
    void		subselect(T&,od_int64 targetsz) const;
			//!< Does not preserve order
			//!< The removed items will really be erased

private:

    int			seed_;

};


mGlobal(Algo) RandGen randGen();


mExpClass(Algo) NormalRandGen : public RandomGenerator
{
public:

			NormalRandGen();

    virtual double	get() const;
    float		get(float expect,float stdev) const;
    double		get(double expect,double stdev) const;

protected:

    mutable bool	useothdrawn_;
    mutable double	othdrawn_;

};


template <class T,class SzTp>
inline void Stats::RandGen::subselect( T* arr, SzTp sz, SzTp targetsz ) const
{
    for ( SzTp idx=sz-1; idx>=targetsz; idx-- )
    {
	const SzTp notselidx = getIndex( idx );
	if ( notselidx != idx )
	    Swap( arr[notselidx], arr[idx] );
    }
}


template <class ODSET>
inline void Stats::RandGen::subselect( ODSET& ods, od_int64 targetsz ) const
{
    typedef typename ODSET::size_type size_type;
    const size_type sz = ods.size();
    if ( sz <= targetsz ) return;

    for ( size_type idx=sz-1; idx>=targetsz; idx-- )
    {
	const size_type notselidx = getIndex( idx );
	if ( notselidx != idx )
	    ods.swap( notselidx, idx );
    }

    removeRange( ods, (size_type)targetsz, sz-1 );
}

}; // namespace Stats


