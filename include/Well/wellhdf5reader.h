#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "wellreadaccess.h"
#include "hdf5reader.h"
#include "uistring.h"
#include "iopar.h"

class IOObj;


namespace Well
{

class HDF5Writer;


/*!\brief stuff common to HDF5 well reader and writer  */

mExpClass(Well) HDF5Access
{
public:

    static const char*		sTrackGrpName();
    static const char*		sLogsGrpName();
    static const char*		sMarkersGrpName();
    static const char*		sTDsGrpName();
    static const char*		sCSsGrpName();
    static const char*		sDispParsGrpName();

    static const char*		sCoordsDSName();
    static const char*		sMDsDSName();
    static const char*		sTWTsDSName();
    static const char*		sValuesDSName();
    static const char*		sNamesDSName();
    static const char*		sColorsDSName();
    static const char*		sLvlIDsDSName();

    static const char*		sKeyLogDel();
};


/*!\brief Reads Well::Data from HDF5 file  */

mExpClass(Well) HDF5Reader : public ReadAccess
			   , public HDF5Access
{ mODTextTranslationClass(Well::HDF5Reader)
public:

			HDF5Reader(const IOObj&,Data&,uiString& errmsg);
			HDF5Reader(const char* fnm,Data&,uiString& errmsg);
			HDF5Reader(const HDF5Writer&,Data&,uiString& errmsg);
			~HDF5Reader();

    bool		getInfo() const override;
    bool		getTrack() const override;
    bool		getLogs(bool needjustinfo=false) const override;
    bool		getMarkers() const override;
    bool		getD2T() const override;
    bool		getCSMdl() const override;
    bool		getDispProps() const override;
    bool		getLog(const char* lognm) const override;
    void		getLogInfo(BufferStringSet&) const override;

    const		uiString& errMsg() const override    { return errmsg_; }

protected:

    uiString&		errmsg_;
    PtrMan<HDF5::Reader> rdr_				= nullptr;
    mutable IOPar	infoiop_;

    void		init(const char*);
    bool		ensureFileOpen() const;
    bool		doGetD2T(bool) const;
    bool		getLogPars(const HDF5::DataSetKey&,IOPar&) const;
    Log*		getWL(const HDF5::DataSetKey&) const;

    bool		get() const override		{ return true; }
};


#define mErrRetIfUiRvNotOK(dsky) \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return false; }

} // namespace Well
