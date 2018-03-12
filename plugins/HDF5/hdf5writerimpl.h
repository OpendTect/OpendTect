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


namespace HDF5
{

mExpClass(HDF5) WriterImpl : public Writer
			   , public AccessImpl
{
public:

			WriterImpl();
    virtual		~WriterImpl();

    const char*		fileName() const	{ return gtFileName(); }

    virtual void	setChunkSize(int);

protected:

    int			chunksz_;

    virtual void	openFile(const char*,uiRetVal&);
    virtual void	closeFile()		{ doCloseFile(*this); }

    virtual void	ptInfo(const DataSetKey&,const IOPar&,uiRetVal&);
    virtual void	ptData(const DataSetKey&,const ArrayNDInfo&,
				const void*,ODDataType,uiRetVal&);

    bool		ensureGroup(const char*);
			//!< returns false if group already existed

};

} // namespace HDF5
