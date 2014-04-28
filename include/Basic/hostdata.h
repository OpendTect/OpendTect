#ifndef hostdata_h
#define hostdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Apr 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "filepath.h"
#include "manobjectset.h"
#include "od_iosfwd.h"
#include "odplatform.h"

class HostDataList;
class IOPar;
class ShareData;

#define mRetNoneIfEmpty( bs ) \
    if ( bs.isEmpty() ) return "_none_"; \
    return bs;

/*!
\brief Host name and aliases.
*/

mExpClass(Basic) HostData
{
public:
			HostData(const char* nm);
			HostData(const char* nm,const HostData& localhost,
				 const OD::Platform&);
			HostData(const HostData&);
    virtual		~HostData();

    enum PathType	{ Appl, Data };

    void		setName( const char* nm )	{ name_ = nm; }
    const char*		name() const			{ return name_; }
    const char*		pass() const	{ mRetNoneIfEmpty(pass_) }

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

    const char*		getDataRoot() const;
    void		setDataRoot(const char*);

    FilePath		convPath( PathType pt, const FilePath&,
				  const HostData* from = 0 ) const;
    FilePath		convPath( PathType pt, const char* fn,
				  const HostData* from = 0 ) const
			{ return convPath(pt, FilePath(fn), from ); }

    static const char*	localHostName();
    void		setLocalHost( const HostData& hd )
			{ localhd_ = &hd; }
    const HostData&	localHost() const
			{ return localhd_ ? *localhd_ : *this; }

    const ShareData*	shareData() const	{ return sharedata_; }
    void		setShareData( const ShareData* sd ) { sharedata_ = sd; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:
			HostData(const char* nm,const OD::Platform&);

    BufferString	name_;
    BufferStringSet	aliases_;
    OD::Platform	platform_;
    FilePath		appl_pr_;
    FilePath		data_pr_;
    BufferString	pass_;
    const HostData*	localhd_;
    const ShareData*	sharedata_;

    friend class	HostDataList;

    void		init( const char* nm );
};


/*!
\brief Describes shared drive and host. Mostly win32.
*/

mExpClass(Basic) ShareData
{
public:
			ShareData( const HostData* hst=0 ) : host_(hst) {}

    const HostData*	host() const	{ return host_; }
    const char*		hostName() const
			{
			    if ( host() ) return host()->name();
			    return "_none_";
			}

    // Windows only
    const char*		drive() const	{ mRetNoneIfEmpty(drive_) }
    const char*		share() const	{ mRetNoneIfEmpty(share_) }
    const char*		pass() const
			{
			    if ( pass_ != "" ) return pass_;
			    if ( host() ) return host()->pass();
			    return "_none_";
			}
protected:


    const HostData*	host_;
    BufferString	drive_;
    BufferString	share_;
    BufferString	pass_;

    friend class	HostDataList;
};



/*!
\brief List of host names in the system.
  The first entry will be the local host.
*/

mExpClass(Basic) HostDataList : public ManagedObjectSet<HostData>
{
public:
			HostDataList(bool readhostfile=true,
				     bool addlocalhost=true);

    void		setNiceLevel(int);
    int			niceLevel() const;
    void		setFirstPort(int);
    int			firstPort() const;
    void		setLoginCmd(const char*);
    const char*		loginCmd() const;

    HostData*		find( const char* nm )	{ return findHost(nm); }
    const HostData*	find( const char* nm ) const { return findHost(nm); }

    void		fill(BufferStringSet&,bool inclocalhost=true) const;
    void		dump(od_ostream&) const;

    const char*		getBatchHostsFilename() const;
    bool		writeHostFile(const char* fnm);

protected:

    BufferString	logincmd_;
    int			nicelvl_;
    int			firstport_;
    FilePath		win_appl_pr_;
    FilePath		unx_appl_pr_;
    FilePath		win_data_pr_;
    FilePath		unx_data_pr_;
    ShareData		sharedata_;

    void		handleLocal();
    bool		readHostFile(const char*);
    bool		readOldHostFile(const char*);
    HostData*		findHost(const char*) const;
    BufferString	batchhostsfnm_;
};

#undef mRetNoneIfEmpty

#endif

