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

			    WriterImpl();
    virtual		    ~WriterImpl();

    const char*		    fileName() const	{ return gtFileName(); }

    virtual void	    setDataType(OD::FPDataRepType);
    virtual int		    chunkSize() const;
    virtual void	    setChunkSize(int);

    virtual uiRetVal	    putInfo(const GroupPath&,const IOPar&);
    virtual uiRetVal	    putData(const GroupPath&,const ArrayND<float>&,
				    const IOPar* iop=0);

protected:

    virtual void	    openFile(const char*,uiRetVal&);
    virtual void	    closeFile()		{ doCloseFile(*this); }

};

} // namespace HDF5
