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

mExpClass(ODHDF5) AccessImpl
{
public:

			AccessImpl(ReaderImpl&);
			AccessImpl(WriterImpl&);
    virtual		~AccessImpl();

    static void		setErrPrint(bool);	//!< user switch on/off

protected:

    const char*		gtFileName() const;
    DataSetKey		gtScope() const;
    od_int64		gtGroupID() const;

    bool		haveGroup() const;
    bool		haveDataSet() const;
    bool		atGroup(const char*&) const;
    bool		atDataSet(const char*) const;

			// no throw
    void		doCloseFile(Access&);

    H5::Group*		selectGroup(const char*) const;
    H5::DataSet*	selectDataSet(const char*) const;
    H5::H5Object*	stScope(const DataSetKey*) const;
    H5::H5Object*	stScope(const DataSetKey*);
    H5::Group*		stGrpScope(const DataSetKey*) const;
    H5::Group*		stGrpScope(const DataSetKey*);
    H5::DataSet*	stDSScope(const DataSetKey&) const;
    H5::DataSet*	stDSScope(const DataSetKey&);

    void		selectSlab(H5::DataSpace&,const SlabSpec&,
				   TypeSet<hsize_t>* pcounts=0) const;
				//!< can throw, use in try block
    static bool		haveErrPrint();

    typedef H5::PredType H5DataType;
    static const H5DataType& h5DataTypeFor(ODDataType);

    Access&		acc_;
    mutable H5::Group	group_;
    mutable H5::DataSet	dataset_;
    mutable ArrayNDInfo::nr_dims_type nrdims_;

private:

			AccessImpl(const AccessImpl&)	= delete;

    static bool		validH5Obj(const H5::H5Object&);

    static void		disableErrPrint(); // before action with 'normal' throw
    static void		restoreErrPrint(); // after such an action
    static void		enableErrPrint();

};


mExpClass(ODHDF5) AccessProviderImpl : public AccessProvider
{
public:

    mDefaultFactoryInstantiation( AccessProvider, AccessProviderImpl,
			"OD", toUiString("OD") );

    Reader*		getReader() const override;
    Writer*		getWriter() const override;

    static void		initHDF5(); //!< class initClass()

};

} // namespace HDF5


#define mGetDataSpaceDims( dims, nrdims, dataspace ) \
    TypeSet<hsize_t> dims( nrdims, (hsize_t)0 ); \
    dataspace.getSimpleExtentDims( dims.arr() )
