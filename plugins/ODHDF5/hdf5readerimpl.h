#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5common.h"
#include "hdf5accessimpl.h"
#include "hdf5reader.h"


namespace HDF5
{

mExpClass(ODHDF5) ReaderImpl : public Reader
			   , public AccessImpl
{
public:

			ReaderImpl();
			ReaderImpl(const H5::H5File&);
			~ReaderImpl();

    const char*		fileName() const	{ return gtFileName(); }

    virtual void	getGroups(BufferStringSet&) const;
    virtual void	getDataSets(const char* grpnm,BufferStringSet&) const;

    virtual DataSetKey	scope() const		{ return gtScope(); }
    virtual bool	setScope( const DataSetKey& dsky )
						{ return stScope( dsky ); }
    virtual od_int64	curGroupID() const	{ return gtGroupID(); }

    virtual ODDataType	getDataType() const;

protected:

    virtual void	openFile(const char*,uiRetVal&,bool);
    virtual void	closeFile()		{ doCloseFile(*this); }

    virtual nr_dims_type gtNrDims() const	{ return nrdims_; }
    virtual ArrayNDInfo* gtDataSizes() const;
    virtual void	gtInfo(IOPar&,uiRetVal&) const;
    virtual void	gtStrings(BufferStringSet&,uiRetVal&) const;
    virtual void	gtAll(void*,uiRetVal&) const;
    virtual void	gtPoints(const NDPosBufSet&,void*,uiRetVal&) const;
    virtual void	gtSlab(const SlabSpec&,void*,uiRetVal&) const;

    template <class H5Dir>
    void		listObjs(const H5Dir&,BufferStringSet&,bool) const;
    const H5DataType&	h5DataType() const;

};

} // namespace HDF5
