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

mExpClass(HDF5) ReaderImpl : public Reader
{
public:

			    ReaderImpl();
			    ~ReaderImpl();

    virtual uiRetVal	    open(const char*);
    virtual void	    getGroups(const GroupPath&,BufferStringSet&) const;
    virtual GroupID	    groupIDFor(const GroupPath&) const;

protected:

    H5::H5File*		    file_;

};

} // namespace HDF5
