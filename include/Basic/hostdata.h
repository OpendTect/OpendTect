#ifndef hostdata_h
#define hostdata_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.h,v 1.14 2004-11-02 16:05:38 arend Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "genc.h"
#include "filepath.h"

class HostDataList;

#define mRetNoneIfEmpty( bs ) \
    if ( bs == "" ) return "_none_"; \
    return bs;

/*\brief Host name and aliases */

class HostData
{
public:

    enum PathType	{ Appl, Data };

protected:
    			HostData( const char* nm, bool iswin=false )
			: name_(nm)
			, iswin_(iswin)
			, datahost_(0)
			, localhd_(0) {}
public:
    			HostData( const char* nm, const HostData& localhost,
				  bool iswin=false )
			: name_(nm)
			, iswin_(iswin)
			, datahost_(0)
			, localhd_(&localhost) {}

    virtual		~HostData()	{ deepErase(aliases_); }

    const char*		name() const	{ return (const char*)name_; }

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
			    { return convPath(pt, fn, from ); }

    static const char*	localHostName();
    void		setLocalHost( const HostData& hd )
			{ localhd_ = &hd; }
    const HostData&	localHost() const
    			{ return localhd_ ? *localhd_ : *this; }

    //! Windows only
    HostData*		dataHost() const	{ return datahost_; }
    const char*		dataDrive() const	{ mRetNoneIfEmpty(datadrive_) }
    const char*		dataShare() const	{ mRetNoneIfEmpty(datashare_) }
    const char*		pass() const		{ mRetNoneIfEmpty(pass_) }

protected:

    BufferString	name_;
    BufferStringSet	aliases_;
    bool		iswin_;
    FilePath		appl_pr_;
    FilePath		data_pr_;
    const HostData*	localhd_;

    HostData*		datahost_;
    BufferString	datadrive_;
    BufferString	datashare_;
    BufferString	pass_;

    friend class	HostDataList;

};


/*\brief List of host names in the system

  The first entry will be the local host.
 
 */

class HostDataList : public ObjectSet<HostData>
{
public:
			HostDataList( bool readhostfile=true );
    virtual		~HostDataList()		{ deepErase(*this); }

    int			defNiceLevel() const	{ return defnicelvl_; }
    int			firstPort() const	{ return portnr_; }
    const char*		rshComm() const		{ return rshcomm_; }

protected:

    bool		realaliases_;
    BufferString	rshcomm_;
    int			defnicelvl_;
    int			portnr_;
    FilePath		win_appl_pr_;
    FilePath		unx_appl_pr_;
    FilePath		win_data_pr_;
    FilePath		unx_data_pr_;
    BufferString	datahost_;
    BufferString	datadrive_;
    BufferString	datashare_;
    BufferString	remotepass_;

    void		handleLocal();
    bool		readHostFile(const char*);
};

#undef mRetNoneIfEmpty

#endif
