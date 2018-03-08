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

    uiRetVal		getData(Reader&,const DataSetKey&);
    uiRetVal		putData(Writer&,const DataSetKey&);

    const ArrayND<T>&	arrnd_;

};


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
