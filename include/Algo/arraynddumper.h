#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2013
________________________________________________________________________


-*/

#include "algomod.h"
#include "arraynd.h"
#include "od_iostream.h"

/*!\brief Dumps contents of ArrayND objects.

  The options are:
  * Newline after how many points?
  * With position info? Will be dumped only at each line start.
  * Can there be undefs? If not, set haveudfs_ to false

 */

template <class T>
mClass(Algo) ArrayNDDumper
{
public:

				mTypeDefArrNDTypes;

				ArrayNDDumper( const ArrayND<T>& arr )
				    : inp_(arr), haveudfs_(true)
				    , withpos_(false)
					{ setOneLinePerFastestDim(); }

    inline void			setSingleLineWithPos()
					{ withpos_ = true; nlafter_ = 1; }
    inline void			setOneLinePerFastestDim();
    inline void			setOneLinePerSlowestDim();

    inline void			dump(od_ostream&) const;

    const ArrayND<T>&		inp_;
    bool			withpos_;
    bool			haveudfs_;
    total_size_type		nlafter_;

};


template <class T>
void ArrayNDDumper<T>::setOneLinePerFastestDim()
{
    const auto nrdims = inp_.nrDims();
    nlafter_ = nrdims < 2 ? 0 : inp_.getSize( nrdims - 1 );
}


template <class T>
void ArrayNDDumper<T>::setOneLinePerSlowestDim()
{
    const auto nrdims = inp_.nrDims();
    if ( nrdims < 2 )
	{ nlafter_ = 0; return; }

    nlafter_ = 1;
    for ( dim_idx_type idim=1; idim<nrdims; idim++ )
	nlafter_ *= inp_.getSize( idim );
}


template <class T>
void ArrayNDDumper<T>::dump( od_ostream& strm ) const
{
    if ( inp_.isEmpty() ) return;

    ArrayNDIter it( inp_.info() );
    const auto nrdims = inp_.nrDims();

    total_size_type nrthisline = 0;
    while ( true )
    {
	NDPos pos = it.getPos();
	if ( withpos_ && nrthisline == 0 )
	{
	    for ( dim_idx_type idim=0; idim<nrdims; idim++ )
		strm.add( pos[idim] ).add( od_tab );

	}

	const T val = inp_.getND( pos );
	if ( haveudfs_ && mIsUdf(val) )
	    strm.add( "udf" );
	else
	    strm.add( val );

	char wschar = od_tab;
	if ( nlafter_ > 0 )
	{
	    nrthisline++;
	    if ( nrthisline == nlafter_ )
		{ wschar = od_newline; nrthisline = 0; }
	}
	if ( it.next() )
	    strm.add( wschar );
	else
	    { strm.add(od_newline).flush(); break; }
    }
}
