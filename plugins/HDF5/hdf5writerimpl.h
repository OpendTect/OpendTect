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
#include "hdf5writer.h"
#include "H5Cpp.h"


namespace HDF5
{

mExpClass(HDF5) WriterImpl : public Writer
			   , public AccessImpl
{
public:

    typedef H5::DataType	H5DataType;

			WriterImpl();
    virtual		~WriterImpl();

    const char*		fileName() const	{ return gtFileName(); }

    virtual void	setChunkSize(int);

protected:

    int			chunksz_;
    static H5DataType	h5DataTypeFor(ODDataType);

    virtual void	openFile(const char*,uiRetVal&);
    virtual void	closeFile()		{ doCloseFile(*this); }

    virtual void	ptInfo(const DataSetKey&,const IOPar&,uiRetVal&);
    virtual void	ptData(const DataSetKey&,const ArrayNDInfo&,
				   const Byte*,ODDataType,uiRetVal&);

    void		ensureGroup(const char*);

};

} // namespace HDF5
