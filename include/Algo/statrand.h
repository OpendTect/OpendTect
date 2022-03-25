#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2006
________________________________________________________________________

-*/

#include "algomod.h"

#include "gendefs.h"
#include "odset.h"

#include <random>


namespace Stats
{

/*!\brief Random Generator */

mExpClass(Algo) RandomGenerator
{
public:

    virtual		~RandomGenerator()	{}

    virtual double	get() const		= 0;	// returns in [0,1)

    // Not for production usage:
    virtual bool	init(int seed)		{ return false; }
				/* seed != 0 gives repeatable numbers
				   Use exclusively with non-zero values when
				   you need reproducibility, like in a test
				   program */


};


/*!\brief Uniform Random Generator
  Not thread-safe to preserve performance.
  In multi-threaded context, use one generator per thread
 */

mExpClass(Algo) RandGen : public RandomGenerator
{
public:

			RandGen();

    double		get() const override;
			//!< Uniform [0-1]
    int			getInt() const;
			//!< Uniform int
    int			getInt(int min,int max) const;
			//!< random int in the range [min,max]>
    int			getIndex(int sz) const;
			//!< random index in the range [0,sz>
    od_int64		getIndex(od_int64 sz) const;
			//!< random index in the range [0,sz>

    template <class T,class SzTp>
    void		subselect(T*,SzTp sz,SzTp targetsz) const;
			//!< Does not preserve order.
			//!< Afterwards, the 'removed' values occupy
			//!< the indexes targetsz - maxsz-1
    template <class T>
    void		subselect(T&,od_int64 targetsz) const;
			//!< Does not preserve order
			//!< The removed items will really be erased

    bool		init(int seed) override;
			//!< Not for production

private:
			RandGen(const RandGen&)			= delete;
    RandGen&		operator =(const RandGen&)		= delete;

    std::random_device	rd_;
    mutable std::mt19937 gen_;
    mutable std::mt19937_64 gen64_;
    Threads::Atomic<int> seedval_ = mUdf(int);

public:
    mDeprecated("Use getIndex")
    int			getIndexFast( int sz, int /*seed*/ ) const
			{ return getIndex(sz); }
    mDeprecated("Use getIndex")
    od_int64		getIndexFast( od_int64 sz, od_int64 /* seed */ ) const
			{ return getIndex(sz); }

};

mDeprecated("Use a local or class member generator")
mGlobal(Algo) RandGen& randGen();


/*!\brief Normal (gaussian) Random Generator
  Not thread-safe to preserve performance.
  In multi-threaded context, use one generator per thread
  */

mExpClass(Algo) NormalRandGen : public RandomGenerator
{
public:

			NormalRandGen();

    double		get() const override;
    float		get(float expect,float stdev) const;
    double		get(double expect,double stdev) const;

    bool		init(int seed) override;
			//!< Not for production

private:
			NormalRandGen(const NormalRandGen&)	= delete;
    NormalRandGen&	operator =(const NormalRandGen&)	= delete;

    std::random_device	rd_;
    mutable std::mt19937_64 gen64_;
    Threads::Atomic<int> seedval_ = mUdf(int);

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


