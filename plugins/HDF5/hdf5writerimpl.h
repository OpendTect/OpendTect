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
    virtual		    ~WriterImpl();

    const char*		    fileName() const	{ return gtFileName(); }

    virtual void	    setDims(const ArrayNDInfo&);
    virtual int		    chunkSize() const;
    virtual void	    setChunkSize(int);

protected:

    virtual void	    openFile(const char*,uiRetVal&);
    virtual void	    closeFile()		{ doCloseFile(*this); }

};

} // namespace HDF5
