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

    virtual bool	putInfoAndTrack() const;
    virtual bool	putLogs() const;
    virtual bool	putMarkers() const;
    virtual bool	putD2T() const;
    virtual bool	putCSMdl() const;
    virtual bool	putDispProps() const;

    virtual const uiString& errMsg() const	{ return errmsg_; }

    virtual bool	put() const;

    HDF5::Reader*	createCoupledHDFReader() const;
    static bool		useHDF5(const IOObj&,uiString&);

protected:

    HDF5::Writer*	wrr_;
    BufferString	filename_;
    uiString&		errmsg_;

    void		init(const char*,bool* nmchg=0);
    void		putDepthUnit(IOPar&) const;
    bool		doPutD2T(bool) const;
    bool		ensureFileOpen() const;
    void		ensureCorrectDSSize(const DataSetKey&, int,int,
					    uiRetVal&) const;
    bool		putLog(int,const Log&,uiRetVal&) const;
    bool		setLogAttribs(const DataSetKey&,const Log*) const;

    virtual bool	isFunctional() const;

};


}; // namespace Well
