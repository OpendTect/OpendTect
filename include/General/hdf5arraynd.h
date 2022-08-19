#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "hdf5reader.h"
#include "hdf5writer.h"
#include "uistrings.h"


namespace HDF5
{

template <class T> class ArrayNDTool
{
public:
			mTypeDefArrNDTypes;

			ArrayNDTool( const ArrayND<T>& arrnd )
			    : arrnd_(const_cast<ArrayND<T>&>(arrnd))	{}
			ArrayNDTool( ArrayND<T>& arrnd )
			    : arrnd_(arrnd)	{}
			~ArrayNDTool()
			    { delete [] workarr_; }

    inline static ArrayND<T>* createArray(const DataSetKey&,Reader&);
    inline uiRetVal	getAll(const DataSetKey&,Reader&);
    inline uiRetVal	getSlab(const DataSetKey&,Reader&,const SlabSpec&);
			    //!< SlabSpec in file goes to (0,0,...) in arr

    inline uiRetVal	createDataSet(Writer&,const DataSetKey&);
			    //!< creates appropriate dataset
    inline uiRetVal	put(Writer&,const DataSetKey&);
			    //!< if necessary, creates dataset
    inline uiRetVal	putAll(Writer&,const DataSetKey&);
    inline uiRetVal	putSlab(Writer&,const DataSetKey&,const SlabSpec&);
			    //!< put data from (0,0,...) into SlabSpec in file
			    //!< writes to current dataset

    ArrayND<T>&		arrnd_;
    T*			workarr_ = nullptr;

    inline T*		getWorkArray(uiRetVal&);

};


template <class T>
inline ArrayND<T>* ArrayNDTool<T>::createArray( const DataSetKey& dsky,
						Reader& rdr )
{
    uiRetVal uirv;
    PtrMan<ArrayNDInfo> inf = rdr.getDataSizes( dsky, uirv );
    if ( !inf )
	return nullptr;

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
inline uiRetVal ArrayNDTool<T>::getAll( const DataSetKey& dsky, Reader& rdr )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    T* arr = getWorkArray( uirv );
    if ( arr )
    {
	uirv = rdr.getAll( dsky, arr );
	if ( workarr_ )
	    arrnd_.setData( arr );
    }

    return uirv;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::getSlab( const DataSetKey& dsky, Reader& rdr,
					 const SlabSpec& spec )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    T* arr = getWorkArray( uirv );
    if ( arr )
    {
	uirv = rdr.getSlab( dsky, spec, arr );
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
    uiRetVal uirv;
    if ( !wrr.hasDataSet(dsky) )
	uirv = createDataSet( wrr, dsky );
    if ( !uirv.isOK() )
	return uirv;

    return putAll( wrr, dsky );
}


template <class T>
inline uiRetVal ArrayNDTool<T>::putAll( Writer& wrr, const DataSetKey& dsky )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    const T* arr = getWorkArray( uirv );
    if ( arr )
    {
	if ( workarr_ )
	    arrnd_.getAll( const_cast<T*>(arr) );
	uirv = wrr.putAll( dsky, arr );
    }

    return uirv;
}


template <class T>
inline uiRetVal ArrayNDTool<T>::putSlab( Writer& wrr, const DataSetKey& dsky,
					 const SlabSpec& spec )
{
    uiRetVal uirv;
    if ( arrnd_.totalSize() < 1 )
	return uirv;

    const T* arr = getWorkArray( uirv );
    if ( arr )
    {
	if ( workarr_ )
	    arrnd_.getAll( const_cast<T*>(arr) );
	uirv = wrr.putSlab( dsky, spec, arr );
    }

    return uirv;
}


} // namespace HDF5
