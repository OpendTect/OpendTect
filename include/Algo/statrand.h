#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    mDeprecated("Use setSeed or clearSeed")
    virtual bool	init(int seed)	{ return false; }


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

    bool		setSeed(int seed);
    void		clearSeed();

private:
			mOD_DisableCopy(RandGen);

    void		seedRandom();
    void		seedFromInt(int seed);

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

    mDeprecated("Use setSeed or clearSeed")
    bool		init(int seed) override;

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
			~NormalRandGen();

    double		get() const override;
    float		get(float expect,float stdev) const;
    double		get(double expect,double stdev) const;

    bool		setSeed(int seed);
    void		clearSeed();

private:
			mOD_DisableCopy(NormalRandGen);

    void		seedRandom();
    void		seedFromInt(int seed);

    std::random_device	rd_;
    mutable std::mt19937_64 gen64_;
    Threads::Atomic<int> seedval_ = mUdf(int);

public:

    mDeprecated("Use setSeed or clearSeed")
    bool		init(int seed) override;

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

} // namespace Stats
