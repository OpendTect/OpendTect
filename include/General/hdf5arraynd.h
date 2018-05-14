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
			~ArrayNDTool()
			    { delete [] workarr_; }

    inline static ArrayND<T>* createArray(Reader&);
    inline uiRetVal	getAll(Reader&);
    inline uiRetVal	getSlab(Reader&,const SlabSpec&);
			    //!< SlabSpec in file goes to (0,0,...) in arr

    inline uiRetVal	put(Writer&,const DataSetKey&);
			    //!< creates dataset and writes the array
    inline uiRetVal	createDataSet(Writer&,const DataSetKey&);
			    //!< creates appropriate dataset
    inline uiRetVal	putAll(Writer&);
			    //!< writes to current dataset
    inline uiRetVal	putSlab(Writer&,const SlabSpec&);
			    //!< put data from (0,0,...) into SlabSpec in file
			    //!< writes to current dataset

    ArrayND<T>&		arrnd_;
    T*			workarr_ = 0;

    inline T*		getWorkArray(uiRetVal&);

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
inline T* ArrayNDTool<T>::getWorkArray( uiRetVal& uirv )
{
    if ( workarr_ )
	return workarr_;

    T* ret = arrnd_.getData();
    if ( ret )
	return ret;

    mTryAlloc( workarr_, T [ arrnd_.totalSize() ] );
    if ( !workarr_ )
	uirv.add( uiStrings::phrCannotAllocateMemory() );

    return workarr_;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::getAll( Reader& rdr )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    T* arr = getWorkArray( uirv );
    if ( arr )
    {
	uirv = rdr.getAll( arr );
	if ( workarr_ )
	    arrnd_.setData( arr );
    }

    return uirv;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::getSlab( Reader& rdr, const SlabSpec& spec )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    T* arr = getWorkArray( uirv );
    if ( arr )
    {
	uirv = rdr.getSlab( spec, arr );
	if ( workarr_ )
	    arrnd_.setAll( arr );
    }

    return uirv;
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
inline uiRetVal ArrayNDTool<T>::put( Writer& wrr, const DataSetKey& dsky )
{
    uiRetVal uirv = createDataSet( wrr, dsky );
    if ( uirv.isOK() )
	uirv = putAll( wrr );
    return uirv;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::putAll( Writer& wrr )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    const T* arr = getWorkArray( uirv );
    if ( arr )
    {
	if ( workarr_ )
	    arrnd_.getAll( const_cast<T*>(arr) );
	uirv = wrr.putAll( arr );
    }

    return uirv;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::putSlab( Writer& wrr, const SlabSpec& spec )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    const T* arr = getWorkArray( uirv );
    if ( arr )
    {
	if ( workarr_ )
	    arrnd_.getAll( const_cast<T*>(arr) );
	uirv = wrr.putSlab( spec, arr );
    }

    return uirv;
}


} // namespace HDF5
