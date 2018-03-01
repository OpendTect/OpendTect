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

			    ReaderImpl();
			    ~ReaderImpl();

    const char*		    fileName() const	{ return gtFileName(); }

    virtual int		    chunkSize() const;
    virtual void	    getGroups(const GroupPath&,BufferStringSet&) const;

protected:

    virtual void	    openFile(const char*,uiRetVal&);
    virtual void	    closeFile()		{ doCloseFile(*this); }

};

} // namespace HDF5
