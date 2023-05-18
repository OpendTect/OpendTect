#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
  a group can be written if the group exists.
* When opened in 'edit' mode, you can get info about the file from a coupled
  Reader. Do not try to use such a Reader when the Writer is closed/destroyed.
* Every group or DataSet can have an IOPar attached with info. Although HDF5
  supports all types of 'attributes', this is the form we support. If there
  are already attributes for the data set they will be completely replaced.
* If you want your data sets to be resizable (for example when data grows, but
  also when opened in edit mode), then you have to use
  setEditableCreation(true) before creating the DataSet. Such datasets are
  always chunked.
* You can extend (or shrink) the DataSet using resizeDataSet(). There is no
  auto-resize going on, so if you think the data set may already exist and the
  dim sizes may be changed, then always use resizeDataSet().

*/

mExpClass(General) Writer : public Access
{ mODTextTranslationClass(Writer)
public:

    uiRetVal		open4Edit(const char*);
			//!< For normal 'create', use 'open()

    virtual void	setCompressionLevel(unsigned)		{}
			/*!< Compression level (0-9)
			See gzip documentation. 0=None */

    virtual H5::Group*	ensureGroup(const char* grpnm,uiRetVal&) = 0;
			//!< Creates a new group if necessary

    uiRetVal		createDataSet(const DataSetKey&,const ArrayNDInfo&,
				      ODDataType);
    uiRetVal		createDataSet(const DataSetKey&,int,ODDataType);
    uiRetVal		createDataSetIfMissing(const DataSetKey&,ODDataType,
				const ArrayNDInfo& addedsz,
				const ArrayNDInfo& changedir,
				PtrMan<ArrayNDInfo>* existsinfo =nullptr);
			//!<param changedir: Array reshaping behavior
			//! for each dimension: -1 = shrink, 0 = no change
			//! 1 = grow
    uiRetVal		createDataSetIfMissing(const DataSetKey&,ODDataType,
				int addedsz,int changedir=1,
				int* existsnrsamples =nullptr);
    uiRetVal		createTextDataSet(const DataSetKey&);
    uiRetVal		resizeDataSet(const DataSetKey&,const ArrayNDInfo&);
			//!< You cannot change the 'rank', just the dim sizes

    uiRetVal		putSlab(const DataSetKey&,const SlabSpec&,const void*);
    uiRetVal		putAll(const DataSetKey&,const void*);
    template <class T>
    uiRetVal		put(const DataSetKey&,const T*,int sz);
    template <class T>
    uiRetVal		put(const DataSetKey&,const TypeSet<T>&);
    uiRetVal		put(const DataSetKey&,const BufferStringSet&);

			// null = root scope
    virtual void	setAttribute(const char* ky,const char* val,
				     const DataSetKey* =nullptr)	= 0;
    uiRetVal		removeAttribute(const char*,
				     const DataSetKey* = nullptr);
    uiRetVal		removeAllAttributes(const DataSetKey* = nullptr);
#define mHDF5DeclFns(fnnm,type) \
    virtual void	fnnm##Attribute(const char*,type,\
					const DataSetKey* =nullptr)	= 0;
			mHDF5DeclFns(set,od_int16);
			mHDF5DeclFns(set,od_uint16);
			mHDF5DeclFns(set,od_int32);
			mHDF5DeclFns(set,od_uint32);
			mHDF5DeclFns(set,od_int64);
			mHDF5DeclFns(set,od_uint64);
			mHDF5DeclFns(set,float);
			mHDF5DeclFns(set,double);
#undef mHDF5DeclFns
    uiRetVal		set(const IOPar&,const DataSetKey* =nullptr);

    bool		deleteObject(const DataSetKey&);
			//!< after deletion, you can't add it again
			//!< usually, you need a resize

    bool		isReader() const override	{ return false; }
    virtual Reader*	createCoupledReader() const			= 0;

private:

    virtual H5::DataSet*	crDS(const DataSetKey&,const ArrayNDInfo&,
				     ODDataType,uiRetVal&)		= 0;
    virtual H5::DataSet*	crTxtDS(const DataSetKey&,uiRetVal&)	= 0;
    virtual void	reSzDS(const ArrayNDInfo&,H5::DataSet&,
				uiRetVal&)				= 0;
    virtual void	ptSlab(const SlabSpec&,const void*,
			       H5::DataSet&,uiRetVal&)	= 0;
    virtual void	ptAll(const void*,H5::DataSet&,uiRetVal&)	= 0;
    virtual void	ptStrings(const BufferStringSet&,
				  H5::Group&,H5::DataSet*,
				  const char* dsnm,uiRetVal&) = 0;
    virtual void	rmAttrib(const char*,H5::H5Object&)		= 0;
    virtual void	rmAllAttribs(H5::H5Object&)			= 0;

    virtual void	ptInfo(const IOPar&,H5::H5Object&,uiRetVal&)	= 0;

    virtual bool	rmObj(const DataSetKey&)			= 0;

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
    else if ( !hasDataSet(dsky) )
	uirv = createDataSet( dsky, sz, OD::GetDataRepType<T>() );
    if ( uirv.isOK() )
	uirv = putAll( dsky, vals );

    return uirv;
}

} // namespace HDF5
