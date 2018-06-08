#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellwriteaccess.h"
#include "welldahobj.h"
#include "uistring.h"
#include "hdf5writer.h"
class IOObj;


namespace Well
{

/*!\brief Writes Well::Data to HDF5 file */

mExpClass(Well) HDF5Writer : public WriteAccess
{ mODTextTranslationClass(Well::HDF5Writer)
public:

    typedef DahObj::size_type	size_type;
    typedef DahObj::idx_type	idx_type;

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

protected:

    HDF5::Writer*	wrr_;
    uiString&		errmsg_;

    void		init(const char*);
    void		putDepthUnit(IOPar&) const;
    bool		doPutD2T(bool) const;

};


}; // namespace Well
