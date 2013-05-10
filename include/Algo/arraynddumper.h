#ifndef arraynddumper_h
#define arraynddumper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2013
 RCS:           $Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "arraynd.h"
#include <iostream>

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

    				ArrayNDDumper( const ArrayND<T>& arr )
				    : inp_(arr), haveudfs_(true)
				    , withpos_(false)
					{ setOneLinePerFastestDim(); }

    inline void			setSingleLineWithPos()
					{ withpos_ = true; nlafter_ = 1; }
    inline void			setOneLinePerFastestDim();
    inline void			setOneLinePerSlowestDim();

    inline void			dump(std::ostream&) const;

    const ArrayND<T>&		inp_;
    bool			withpos_;
    bool			haveudfs_;
    od_int64			nlafter_;

};


template <class T>
void ArrayNDDumper<T>::setOneLinePerFastestDim()
{
    const int nrdims = inp_.info().getNDim();
    nlafter_ = nrdims < 2 ? 0 : inp_.info().getSize( nrdims - 1 );
}


template <class T>
void ArrayNDDumper<T>::setOneLinePerSlowestDim()
{
    const int nrdims = inp_.info().getNDim();
    if ( nrdims < 2 )
	{ nlafter_ = 0; return; }

    nlafter_ = 1;
    for ( int idim=1; idim<nrdims; idim++ )
	nlafter_ *= inp_.info().getSize( idim );
}


template <class T>
void ArrayNDDumper<T>::dump( std::ostream& strm ) const
{
    if ( inp_.isEmpty() ) return;

    ArrayNDIter it( inp_.info() );
    const int nrdims = inp_.info().getNDim();

    od_int64 nrthisline = 0;
    while ( true )
    {
	const int* pos = it.getPos();
	if ( withpos_ && nrthisline == 0 )
	{
	    for ( int idim=0; idim<nrdims; idim++ )
		strm << pos[idim] << '\t';

	}

	const T val = inp_.getND( pos );
	if ( haveudfs_ && mIsUdf(val) )
	    strm << "udf";
	else
	    strm << val;

	char wschar = '\t';
	if ( nlafter_ > 0 )
	{
	    nrthisline++;
	    if ( nrthisline == nlafter_ )
		{ wschar = '\n'; nrthisline = 0; }
	}
	if ( it.next() )
	    strm << wschar;
	else
	    { strm << std::endl; break; }
    }
}


#endif

