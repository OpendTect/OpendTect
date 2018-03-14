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

    virtual bool	setScope(const DataSetKey&);

    virtual ArrayNDInfo* getDataSizes() const;
    virtual ODDataType	getDataType() const;

protected:

    BufferStringSet	grpnms_;
    mutable H5::Group*	group_;
    mutable H5::DataSet* dataset_;

    virtual void	openFile(const char*,uiRetVal&);
    virtual void	closeFile();

    virtual void	gtInfo(IOPar&,uiRetVal&) const;
    virtual void	gtAll(void*,uiRetVal&) const;
    virtual void	gtPoints(const NDPosSet&,void*,uiRetVal&) const;
    virtual void	gtSlab(const IdxRgSet&,void*,uiRetVal&) const;

    void		listObjs(const H5Dir&,BufferStringSet&,bool) const;
    bool		selectGroup(const char*);
    bool		selectDataSet(const char*);
    inline bool		haveScope( bool needds=true ) const
			{ return group_ && (!needds || dataset_); }
    H5DataType		h5DataType() const;

};

} // namespace HDF5
