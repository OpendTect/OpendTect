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

class HostDataList;
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

    enum PathType	{ Appl, Data };

protected:
    			HostData( const char* nm, bool iswin=false )
			    : iswin_(iswin)
			    , localhd_(0)
			    , sharedata_(0)	{ init(nm); }
public:
    			HostData( const char* nm, const HostData& localhost,
				  bool iswin=false )
			    : iswin_(iswin)
			    , localhd_(&localhost)
			    , sharedata_(0)	{ init(nm); }


    			HostData( const HostData& oth )
			    : aliases_( oth.aliases_ )
			    , iswin_( oth.iswin_ )
			    , appl_pr_( oth.appl_pr_ )
			    , data_pr_( oth.data_pr_ )
			    , pass_( oth.pass_ )
			    , localhd_( oth.localhd_ )
			    , sharedata_( oth.sharedata_ ) { init(oth.name_); }

    virtual		~HostData()	{ deepErase(aliases_); }

    const char*		name() const	{ return (const char*)name_; }
    const char*		pass() const	{ mRetNoneIfEmpty(pass_) }

    int			nrAliases() const
			{ return aliases_.size(); }
    const char*		alias( int idx ) const
			{ return (const char*)(*aliases_[idx]); }
    bool		isKnownAs(const char*) const;
    			//!< true if name or an alias matches
    void		addAlias(const char*);
    			//!< only adds if !isKnownAs

    bool		isWin() const 		{ return iswin_; }
    FilePath::Style	pathStyle() const
			    {
				return iswin_ ? FilePath::Windows
					      : FilePath::Unix;
			    }
			//! As is on remote host.
    const FilePath&	prefixFilePath( PathType pt ) const
			    { return pt == Appl ? appl_pr_ : data_pr_; }

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

protected:

    BufferString	name_;
    BufferStringSet	aliases_;
    bool		iswin_;
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
			HostDataList(bool readhostfile=true);

    int			defNiceLevel() const	{ return defnicelvl_; }
    int			firstPort() const	{ return portnr_; }
    const char*		rshComm() const		{ return rshcomm_; }

    HostData*		find( const char* nm )	{ return findHost(nm); }
    const HostData*	find( const char* nm ) const { return findHost(nm); }

protected:

    bool		realaliases_;
    BufferString	rshcomm_;
    int			defnicelvl_;
    int			portnr_;
    FilePath		win_appl_pr_;
    FilePath		unx_appl_pr_;
    FilePath		win_data_pr_;
    FilePath		unx_data_pr_;
    ShareData		sharedata_;

    void		handleLocal();
    bool		readHostFile(const char*);
    HostData*		findHost(const char*) const;

public:

    void		dump(std::ostream&) const;

};

#undef mRetNoneIfEmpty

#endif

