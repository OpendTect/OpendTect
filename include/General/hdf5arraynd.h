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

template <class T> class ArrayNDTool
{
public:
			mTypeDefArrNDTypes;

			ArrayNDTool( const ArrayND<T>& arrnd )
			    : arrnd_(arrnd)				{}
			ArrayNDTool( ArrayND<T>& arrnd )
			    : arrnd_(const_cast<ArrayND<T>&>(arrnd))	{}

    inline static ArrayND<T>* createArray(Reader&);
    inline uiRetVal	getAll(Reader&);

    inline uiRetVal	put(Writer&,const DataSetKey&);
			//!< creates dataset and writes the array
    inline uiRetVal	createDataSet(Writer&,const DataSetKey&);
			//!< creates appropriate dataset
    inline uiRetVal	putAll(Writer&);
			//!< writes to current dataset

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
    const TotalSzType nrelems = arrnd_.totalSize();
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
inline uiRetVal ArrayNDTool<T>::createDataSet( Writer& wrr,
					       const DataSetKey& dsky )
{
    uiRetVal uirv;
    uirv = wrr.createDataSet( dsky, arrnd_.info(), OD::GetDataRepType<T>() );
    return uirv;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::putAll( Writer& wrr )
{
    uiRetVal uirv;
    const ArrayNDInfo& inf = arrnd_.info();
    const TotalSzType nrelems = inf.totalSize();
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

    uirv = wrr.putAll( data );
    if ( !arrhasdata )
	delete [] data;
    return uirv;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::put( Writer& wrr, const DataSetKey& dsky )
{
    uiRetVal uirv = createDataSet( wrr, dsky );
    if ( uirv.isOK() )
	uirv = putAll( wrr );
    return uirv;
}


} // namespace HDF5
