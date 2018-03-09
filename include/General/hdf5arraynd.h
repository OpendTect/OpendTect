#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2018
________________________________________________________________________

-*/

#include "hdf5reader.h"
#include "hdf5writer.h"
#include "arrayndimpl.h"
#include "uistrings.h"


namespace HDF5
{

template <class T>
mExpClass(General) ArrayNDTool
{
public:

			ArrayNDTool( const ArrayND<T>& arrnd )
			    : arrnd_(arrnd)				{}
			ArrayNDTool( ArrayND<T>& arrnd )
			    : arrnd_(const_cast<ArrayND<T>&>(arrnd))	{}

    inline static ArrayND<T>* createArray(Reader&);
    inline uiRetVal	getAll(Reader&);

    inline uiRetVal	putData(Writer&,const DataSetKey&);

    ArrayND<T>&		arrnd_;

};


template <class T>
inline ArrayND<T>* ArrayNDTool<T>::createArray( Reader& rdr )
{
    PtrMan<ArrayNDInfo> inf = rdr.getDataSizes();
    if ( !inf )
	return 0;

    return ArrayNDImpl<T>::create( *inf );
}


template <class T>
inline uiRetVal ArrayNDTool<T>::getAll( Reader& rdr )
{
    uiRetVal uirv;
    const od_int64 nrelems = arrnd_.totalSize();
    if ( nrelems < 1 )
	return uirv;

    T* data = arrnd_.getData();
    const bool arrhasdata = data;
    if ( !arrhasdata )
    {
	mTryAlloc( data, T [ nrelems ] );
	if ( !data )
	    { uirv.add( uiStrings::phrCannotAllocateMemory() ); return uirv; }
    }
    uirv = rdr.getAll( data );
    if ( !arrhasdata )
    {
	arrnd_.setAll( data );
	delete [] data;
    }
}


template <class T>
inline uiRetVal ArrayNDTool<T>::putData( Writer& wrr, const DataSetKey& dsky )
{
    uiRetVal uirv;
    const ArrayNDInfo& inf = arrnd_.info();
    const od_int64 nrelems = inf.totalSize();
    if ( nrelems < 1 )
	return uirv;

    const T* data = arrnd_.getData();
    const bool arrhasdata = data;
    if ( !arrhasdata )
    {
	mTryAlloc( data, T [ nrelems ] );
	if ( !data )
	    { uirv.add( uiStrings::phrCannotAllocateMemory() ); return uirv; }
	arrnd_.getAll( const_cast<T*>(data) );
    }

    uirv = wrr.putData( dsky, inf, data, OD::GetDataRepType<T>() );
    if ( !arrhasdata )
	delete [] data;
    return uirv;
}

} // namespace HDF5
