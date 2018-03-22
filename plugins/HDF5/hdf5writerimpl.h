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
{ mODTextTranslationClass(HDF5::Writer)
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
    virtual void	crDS(const DataSetKey&,const ArrayNDInfo&,ODDataType,
			     uiRetVal&);
    virtual void	ptAll(const void*,uiRetVal&);
    virtual void	ptSlab(const SlabSpec&,const void*,uiRetVal&);

    bool		ensureGroup(const char*);
			//!< returns false if group already existed

};

} // namespace HDF5
