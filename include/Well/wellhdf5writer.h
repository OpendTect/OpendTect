#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellwriteaccess.h"
#include "wellhdf5reader.h"
#include "hdf5writer.h"


namespace Well
{

/*!\brief Writes Well::Data to HDF5 file */

mExpClass(Well) HDF5Writer : public WriteAccess
			   , public HDF5Access
{ mODTextTranslationClass(Well::HDF5Writer)
public:

			HDF5Writer(const IOObj&,const Data&,uiString& errmsg);
			HDF5Writer(const char* fnm,const Data&,uiString&);
			~HDF5Writer();

    bool		putInfoAndTrack() const override;
    bool		putLogs() const override;
    bool		putMarkers() const override;
    bool		putD2T() const override;
    bool		putCSMdl() const override;
    bool		putDispProps() const override;

    const uiString&	errMsg() const override		{ return errmsg_; }

    bool		put() const override;

    HDF5::Reader*	createCoupledHDFReader() const;
    static bool		useHDF5(const IOObj&,uiString&);

protected:

    PtrMan<HDF5::Writer> wrr_				= nullptr;
    BufferString	filename_;
    uiString&		errmsg_;

    void		init(const char*,bool* nmchg=nullptr);
    bool		initGroups();
    void		putDepthUnit(IOPar&) const;
    bool		doPutD2T(bool) const;
    bool		ensureFileOpen() const;
    void		ensureCorrectDSSize(const HDF5::DataSetKey&,int,int,
					    uiRetVal&) const;
    int			getLogIndex(const char* lognm ) const;
    bool		putLog(const Log&) const override;
    bool		setLogAttribs(const HDF5::DataSetKey&,const Log*) const;

    bool		isFunctional() const override;

};

} // namespace Well
