#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5common.h"
#include "H5Cpp.h"


namespace HDF5
{

class ReaderImpl;
class WriterImpl;

//!brief Mixin for common stuff

mExpClass(HDF5) AccessImpl
{
public:

			AccessImpl(ReaderImpl&);
			AccessImpl(WriterImpl&);
    virtual		~AccessImpl();

    const char*		gtFileName() const;
    DataSetKey		gtScope() const;
    od_int64		gtGroupID() const;

    static bool		haveErrPrint();
    static void		setErrPrint(bool);	//!< user switch on/off

protected:

    typedef H5::DataType H5DataType;

			AccessImpl(const AccessImpl&)	= delete;

    Access&		acc_;
    mutable H5::Group	group_;
    mutable H5::DataSet	dataset_;
    mutable ArrayNDInfo::NrDimsType nrdims_;

			// no throw
    void		doCloseFile(Access&);
    static H5::DataType	h5DataTypeFor(ODDataType);
    bool		atGroup(const char*&) const;
    bool		atDataSet(const char*) const;
    bool		selectGroup(const char*);
    bool		selectDataSet(const char*);
    bool		stScope(const DataSetKey&);
    bool		haveScope(bool needds=true) const;
    bool		haveGroup() const;
    bool		haveDataSet() const;
    void		selectSlab(H5::DataSpace&,const SlabSpec&,
				   TypeSet<hsize_t>* pcounts=0) const;
				//!< can throw, use in try block

    static bool		validH5Obj(const H5::H5Object&);

    static void		disableErrPrint(); // before action with 'normal' throw
    static void		restoreErrPrint(); // after such an action

private:

    static void		enableErrPrint();

};


mExpClass(HDF5) AccessProviderImpl : public AccessProvider
{
public:

    mDefaultFactoryInstantiation0Param( AccessProvider, AccessProviderImpl,
			"OD", toUiString("OD") );

    virtual Reader*	getReader() const;
    virtual Writer*	getWriter() const;

    static void		initHDF5(); //!< class initClass()

};

} // namespace HDF5


#define mGetDataSpaceDims( dims, nrdims, dataspace ) \
    TypeSet<hsize_t> dims( nrdims, (hsize_t)0 ); \
    dataspace.getSimpleExtentDims( dims.arr() )
