#ifndef hostdata_h
#define hostdata_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.h,v 1.13 2004-10-05 14:19:53 dgb Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "genc.h"


/*\brief Host name and aliases */

class HostData
{
public:

    				HostData( const char* nm, bool iswin=false )
				: name_(nm), status(0) , nrdone(0), nrfail(0)
				, iswin_(iswin) {}

    virtual			~HostData()	{ deepErase(aliases_); }

    const char*			name() const	{ return (const char*)name_; }

    int				nrAliases() const
				{ return aliases_.size(); }
    const char*			alias( int idx ) const
				{ return (const char*)(*aliases_[idx]); }
    bool			isKnownAs(const char*) const;
    				//!< true if name or an alias matches
    void			addAlias(const char*);
    				//!< only adds if !isKnownAs

    bool			isWin() const	{ return iswin_; }
    const char*			applPrefix() const
				{
				    if ( applprefix_ != "" ) return applprefix_;
				    return 0;
				}
    const char*			dataPrefix() const
				{
				    if ( dataprefix_ != "" ) return dataprefix_;
				    return 0;
				}

    static const char*		localHostName();

    int				status; 
    int				nrdone;
    int				nrfail;

protected:

    BufferString		name_;
    BufferStringSet		aliases_;
    bool			iswin_;
    BufferString		applprefix_;
    BufferString		dataprefix_;

    friend class		HostDataList;

};


/*\brief List of host names in the system

  The first entry will be the local host.
 
 */

class HostDataList : public ObjectSet<HostData>
{
public:
			HostDataList();
    virtual		~HostDataList()	{ deepErase(*this); }

    int			defNiceLevel() const	{ return defnicelvl_; }
    int			firstPort() const	{ return portnr_; }
    const char*		rshComm() const		{ return rshcomm_; }

    //! Windows only
    const char*		dataHost() const	{ return datahost_; }
    const char*		dataDrive() const	{ return datadrive_; }
    const char*		dataShare() const	{ return datashare_; }
    const char*		remotePass() const	{ return remotepass_; }

    const char*		applPrefix( const HostData& host ) const
			{
			    if ( host.applPrefix() ) 
				return  host.applPrefix();

			    const char* prefx = host.isWin() ?
				win_appl_prefix_ : unx_appl_prefix_;

			    if ( !prefx || !*prefx )
				return GetSoftwareDir(); 

			    return prefx;
			}

    const char*		dataPrefix( const HostData& host ) const
			{
			    if ( host.dataPrefix() ) 
				return  host.dataPrefix();

			    const char* prefx = host.isWin() ?
				win_data_prefix_ : unx_data_prefix_;

			    if ( !prefx || !*prefx )
				return GetSoftwareDir(); 

			    return prefx;
			}

    const char*		getRemoteDataFileName( const char* fn,
					const HostData& host, bool native );
    const char*		getRemoteDataDir(  const HostData& host, bool native );
    const char*		getRemoteApplDir(  const HostData& host, bool native );

protected:

    bool		realaliases_;
    BufferString	rshcomm_;
    int			defnicelvl_;
    int			portnr_;
    BufferString	win_appl_prefix_;
    BufferString	unx_appl_prefix_;
    BufferString	win_data_prefix_;
    BufferString	unx_data_prefix_;
    BufferString	datahost_;
    BufferString	datadrive_;
    BufferString	datashare_;
    BufferString	remotepass_;

    void		handleLocal();
    bool		readHostFile(const char*);
};


#endif
