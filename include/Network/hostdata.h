#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2002
________________________________________________________________________

-*/

#include "networkmod.h"

#include "bufstringset.h"
#include "filepath.h"
#include "manobjectset.h"
#include "od_iosfwd.h"
#include "odplatform.h"
#include "uistring.h"

class HostDataList;
class IOPar;
class ShareData;

#define mRetNoneIfEmpty( bs ) \
    if ( bs.isEmpty() ) return "_none_"; \
    return bs;

/*!
\brief Host name and aliases.
*/

mExpClass(Network) HostData
{ mODTextTranslationClass(HostData)
public:
			HostData(const char* nm);
			HostData(const char* nm,const HostData& localhost,
				 const OD::Platform&);
			HostData(const HostData&);
    virtual		~HostData();

    enum PathType	{ Appl, Data };

    void		setHostName(const char*);
    const char*		getHostName() const;
    void		setIPAddress(const char*);
    const char*		getIPAddress() const;

    int			nrAliases() const
			{ return aliases_.size(); }
    void		setAlias(const char*);
    const char*		alias( int idx ) const
			{ return aliases_.get(idx); }
    bool		isKnownAs(const char*) const;
			//!< true if name or an alias matches
    void		addAlias(const char*);
			//!< only adds if !isKnownAs
    BufferString	getFullDispString() const;

    void		setPlatform(const OD::Platform&);
    const OD::Platform& getPlatform() const;
    bool		isWindows() const;
    File::Path::Style	pathStyle() const;
			//! As is on remote host.
    const File::Path&	prefixFilePath(PathType) const;

    const File::Path&	getDataRoot() const;
    void		setDataRoot(const File::Path&);

    File::Path		convPath( PathType pt, const File::Path&,
				  const HostData* from = 0 ) const;
    File::Path		convPath( PathType pt, const char* fn,
				  const HostData* from = 0 ) const
			{ return convPath(pt, File::Path(fn), from ); }

    void		setLocalHost( const HostData& hd )
			{ localhd_ = &hd; }
    const HostData&	localHost() const
			{ return localhd_ ? *localhd_ : *this; }

    uiRetVal		check() const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:
			HostData(const char* nm,const OD::Platform&);

    BufferString	hostname_;
    BufferString	ipaddress_;
    BufferStringSet	aliases_;
    OD::Platform	platform_;
    File::Path		appl_pr_;
    File::Path		data_pr_;
    const HostData*	localhd_;

    friend class	HostDataList;

    void		init( const char* nm );
};



/*!
\brief List of host names in the system.
  The first entry will be the local host.
*/

mExpClass(Network) HostDataList : public ManagedObjectSet<HostData>
{ mODTextTranslationClass(HostDataList)
public:
			HostDataList(bool foredit);

    void		setNiceLevel(int);
    int			niceLevel() const;
    float		priorityLevel() const;
    void		setFirstPort(int);
    int			firstPort() const;
    void		setLoginCmd(const char*);
    const char*		loginCmd() const;
    void		setUnixDataRoot(const char*);
    const char*		unixDataRoot() const;
    void		setWinDataRoot(const char*);
    const char*		winDataRoot() const;

    bool		refresh(bool foredit=false);

    HostData*		find( const char* nm )	{ return findHost(nm); }
    const HostData*	find( const char* nm ) const { return findHost(nm); }

    void		fill(BufferStringSet&,bool inclocalhost=true) const;
    void		dump(od_ostream&) const;

    const char*		getBatchHostsFilename() const;
    bool		writeHostFile(const char* fnm);
    void		fillFromNetwork(); // Unix only

    uiRetVal		check() const;

protected:

    BufferString	logincmd_;
    float		prioritylevel_;
    int			firstport_;
    BufferString	win_appl_pr_;
    BufferString	unx_appl_pr_;
    BufferString	win_data_pr_;
    BufferString	unx_data_pr_;

    void		handleLocal();
    void		initDataRoot();
    bool		readHostFile(const char*);
    bool		readOldHostFile(const char*);
    HostData*		findHost(const char*) const;
    BufferString	batchhostsfnm_;
};

#undef mRetNoneIfEmpty
