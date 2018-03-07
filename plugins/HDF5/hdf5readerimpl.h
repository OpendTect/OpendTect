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
#include "H5Cpp.h"


namespace HDF5
{

mExpClass(HDF5) ReaderImpl : public Reader
			   , public AccessImpl
{
public:

    typedef H5::DataType	H5DataType;
    typedef H5::CommonFG	H5Dir;

			ReaderImpl();
			~ReaderImpl();

    const char*		fileName() const	{ return gtFileName(); }

    virtual void	getGroups(BufferStringSet&) const;
    virtual void	getDataSets(const char* grpnm,BufferStringSet&) const;
    virtual ArrayNDInfo* getDataSizes(const DataSetKey&) const;

protected:

    BufferStringSet	grpnms_;

    virtual void	openFile(const char*,uiRetVal&);
    virtual void	closeFile();

    void		listObjs(const H5Dir&,BufferStringSet&,bool) const;

};

} // namespace HDF5
