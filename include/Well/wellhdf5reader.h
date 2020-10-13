#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2018
________________________________________________________________________


-*/

#include "wellreadaccess.h"
#include "welldahobj.h"
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

    typedef DahObj::size_type	size_type;
    typedef DahObj::idx_type	idx_type;
    typedef DahObj::ZType	ZType;
    typedef HDF5::DataSetKey	DataSetKey;

    static const char*		sLogsGrpName();
    static const char*		sMarkersGrpName();
    static const char*		sDispParsGrpName();

    static const char*		sTrackDSName();
    static const char*		sD2TDSName();
    static const char*		sCSMdlDSName();
    static const char*		sMDsDSName();
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

    virtual bool	getInfo() const;
    virtual bool	getTrack() const;
    virtual bool	getLogs(bool needjustinfo=false) const;
    virtual bool	getMarkers() const;
    virtual bool	getD2T() const;
    virtual bool	getCSMdl() const;
    virtual bool	getDispProps() const;
    virtual bool	getLog(const char* lognm) const;
    virtual void	getLogNames(BufferStringSet&) const;
    virtual void	getLogInfo(ObjectSet<IOPar>&) const;

    virtual const uiString& errMsg() const	{ return errmsg_; }

protected:

    uiString&		errmsg_;
    HDF5::Reader*	rdr_;
    mutable IOPar	infoiop_;

    void		init(const char*);
    bool		ensureFileOpen() const;
    bool		doGetD2T(bool) const;
    bool		getLogPars(const DataSetKey&,IOPar&) const;
    Log*		getWL(const DataSetKey&) const;

};


#define mErrRetIfUiRvNotOK(dsky) \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return false; }

#define mEnsureFileOpen() \
    if ( !ensureFileOpen() ) \
	return false

}; // namespace Well
