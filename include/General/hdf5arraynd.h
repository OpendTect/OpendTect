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
			    : arrnd_(arrnd)			{}

    inline static ArrayND<T>* createArray(Reader&,const DataSetKey&);
    inline bool		getData(Reader&,const DataSetKey&);
    inline uiRetVal	putData(Writer&,const DataSetKey&);

    const ArrayND<T>&	arrnd_;

};


template <class T>
inline ArrayND<T>* ArrayNDTool<T>::createArray( Reader& rdr,
						const DataSetKey& dsky )
{
    PtrMan<ArrayNDInfo> inf = rdr.getDataSizes( dsky );
    if ( !inf )
	return 0;

    return ArrayNDImpl<T>::create( *inf );
}


template <class T>
inline bool ArrayNDTool<T>::getData( Reader& rdr, const DataSetKey& dsky )
{
    PtrMan<ArrayNDInfo> inf = rdr.getDataSizes( dsky );
    if ( !inf )
	return false;

    if ( *inf != arrnd_.info() )
	{ pErrMsg("Fect array has bad dims"); return false; }


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
