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

    virtual const char*	fileName() const	{ return gtFileName(); }

    virtual DataSetKey	scope() const		{ return gtScope(); }
    virtual bool	setScope( const DataSetKey& dsky )
						{ return stScope( dsky ); }

    virtual void	setChunkSize(int);

protected:

    int			chunksz_;

    virtual void	openFile(const char*,uiRetVal&);
    virtual void	closeFile()		{ doCloseFile(*this); }

    virtual void	crDS(const DataSetKey&,const ArrayNDInfo&,ODDataType,
			     uiRetVal&);
    virtual void	ptInfo(const IOPar&,uiRetVal&,const DataSetKey*);
    virtual void	ptAll(const void*,uiRetVal&);
    virtual void	ptSlab(const SlabSpec&,const void*,uiRetVal&);

    bool		ensureGroup(const char*,uiRetVal&);
    void		putAttrib(H5::DataSet&,const IOPar&, uiRetVal&);

};

} // namespace HDF5
