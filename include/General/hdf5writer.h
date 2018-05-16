#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2018
________________________________________________________________________

-*/

#include "hdf5access.h"


namespace HDF5
{

/*!\brief writes to HDF5 file

Notes:
* The Writer has a notion of a 'current' DataSet. Using 'setScope' implies
  that you have created the Data Set first.
* You can write info for an entire group by simpy leaving the dataset name
  empty in the DataSetKey.
* To be able to put info for a DataSet, you have to create it first. Info for
  a group can be written at all times.

  */

mExpClass(General) Writer : public Access
{
public:

    virtual void	setChunkSize(int)				= 0;

    uiRetVal		createDataSet(const DataSetKey&,const ArrayNDInfo&,
				      ODDataType);
    uiRetVal		createDataSet(const DataSetKey&,int,ODDataType);

    uiRetVal		putInfo(const IOPar&); //!< current scope only
    uiRetVal		putInfo(const DataSetKey&,const IOPar&);
			    //!< also for file/group info

    template <class T>
    uiRetVal		put(const DataSetKey&,const T*,int sz);
    template <class T>
    uiRetVal		put(const DataSetKey&,const TypeSet<T>&);
    uiRetVal		put(const DataSetKey&,const BufferStringSet&);

    uiRetVal		putAll(const void*);
    uiRetVal		putSlab(const SlabSpec&,const void*);

protected:

    virtual void	crDS(const DataSetKey&,const ArrayNDInfo&,
				ODDataType,uiRetVal&)			= 0;
    virtual void	ptStrings(const DataSetKey&,const BufferStringSet&,
				  uiRetVal&)				= 0;
    virtual void	ptInfo(const IOPar&,uiRetVal&,const DataSetKey*)= 0;
    virtual void	ptAll(const void*,uiRetVal&)			= 0;
    virtual void	ptSlab(const SlabSpec&,const void*,uiRetVal&)	= 0;

};


template <class T>
inline uiRetVal	Writer::put( const DataSetKey& dsky, const TypeSet<T>& vals )
{
    return put( dsky, vals.arr(), vals.size() );
}


template <class T>
inline uiRetVal	Writer::put( const DataSetKey& dsky, const T* vals, int sz )
{
    uiRetVal uirv;
    if ( !vals )
	return uirv;

    uirv = createDataSet( dsky, sz, OD::GetDataRepType<T>() );
    if ( uirv.isOK() )
	uirv = putAll( vals );

    return uirv;
}

} // namespace HDF5
