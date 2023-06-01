#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    HostData&		operator=(const HostData&);

    enum PathType	{ Appl, Data };

    bool		isStaticIP() const;
    void		setHostName(const char*);
    const char*		getHostName(bool full=true) const;
    void		setIPAddress(const char*);
    const char*		getIPAddress() const;
    BufferString	connAddress() const;
			//! Host name or IP address to be used for a connection

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
    FilePath::Style	pathStyle() const;
			//! As is on remote host.
    const FilePath&	prefixFilePath(PathType) const;

    const FilePath&	getDataRoot() const;
    void		setDataRoot(const FilePath&);

    FilePath		convPath( PathType pt, const FilePath&,
				  const HostData* from = 0 ) const;
    FilePath		convPath( PathType pt, const char* fn,
				  const HostData* from = 0 ) const
			{ return convPath(pt, FilePath(fn), from ); }

    static const char*	localHostName();
			//!< shortcut to GetLocalHostName()

    void		setLocalHost( const HostData& hd )
			{ localhd_ = &hd; }
    const HostData&	localHost() const
			{ return localhd_ ? *localhd_ : *this; }
    bool		isLocalHost() const;

    mDeprecated("Provide localaddr")
    bool		isOK(uiString& errmsg) const;
    bool		isOK(uiString& errmsg,const char* defaultdataroot,
			     const char* localaddr,int prefixlength) const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:
			HostData(const char* nm,const OD::Platform&);

    bool		staticip_;
    BufferString	hostname_;
    BufferString	ipaddress_;
    BufferStringSet	aliases_;
    OD::Platform	platform_;
    FilePath		appl_pr_;
    FilePath		data_pr_;
    const HostData*	localhd_ = nullptr;

    friend class	HostDataList;

    void		init(const char* nm);
};



/*!
\brief List of host names in the system.
  The first entry will be the local host.
*/

mExpClass(Network) HostDataList : public ManagedObjectSet<HostData>
{ mODTextTranslationClass(HostDataList)
public:
			HostDataList(bool foredit);
			~HostDataList();

    void		setNiceLevel(int);
    int			niceLevel() const;
    void		setFirstPort(PortNr_Type);
    PortNr_Type		firstPort() const;
    void		setLoginCmd(const char*);
    const char*		loginCmd() const;
    void		setUnixDataRoot(const char*);
    const char*		unixDataRoot() const;
    void		setWinDataRoot(const char*);
    const char*		winDataRoot() const;

    bool		refresh(bool foredit=false);

    const HostData*	localHost() const;
    bool		isMostlyStaticIP() const;

    HostData*		find( const char* nm )	{ return findHost(nm); }
    const HostData*	find( const char* nm ) const { return findHost(nm); }

    void		fill(BufferStringSet&,bool inclocalhost=true) const;
    void		dump(od_ostream&) const;

    const char*		getBatchHostsFilename() const;
    bool		writeHostFile(const char* fnm);
    void		fillFromNetwork(); // Unix only
    mDeprecated("Provide testall argument")
    bool		isOK(uiStringSet&) const;
    bool		isOK(uiStringSet&,bool testall,
			     BufferString* localaddr =nullptr,
			     int* prefixlength =nullptr) const;

protected:

    BufferString	logincmd_ = "ssh";
    int			nicelvl_ = 19;
    PortNr_Type		firstport_ = 37500;
    BufferString	win_appl_pr_;
    BufferString	unx_appl_pr_;
    BufferString	win_data_pr_;
    BufferString	unx_data_pr_;

    void		handleLocal();
    void		initDataRoot();
    bool		readHostFile(const char*);
    bool		readOldHostFile(const char*);
    HostData*		findHost(const char*);
    const HostData*	findHost(const char*) const;
    BufferString	batchhostsfnm_;

};

#undef mRetNoneIfEmpty
