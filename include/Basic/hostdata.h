#ifndef hostdata_h
#define hostdata_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.h,v 1.11 2004-06-30 13:01:08 arend Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"


/*\brief Host name and aliases */

class HostData
{
public:

    				HostData( const char* nm, bool ra=true )
				: name_(nm), realaliases_(ra), status(0)
				, nrdone(0), nrfail(0) {}
    virtual			~HostData()	{ deepErase(aliases_); }

    const char*			name() const	{ return (const char*)name_; }
    const char*			applPrefix() const
				    { return (const char*)appl_prefix; }
    const char*			dataPrefix() const
				    { return (const char*)data_prefix; }
    int				nrAliases() const
				{ return aliases_.size(); }
    const char*			alias( int idx ) const
				{ return (const char*)(*aliases_[idx]); }
    bool			isKnownAs(const char*) const;
    				//!< true if name or an alias matches
    void			addAlias(const char*);
    				//!< only adds if !isKnownAs

    static const char*		localHostName();

    int				status; 
    int				nrdone;
    int				nrfail;

protected:

    BufferString		name_;
    BufferStringSet		aliases_;
    BufferString		appl_prefix;
    BufferString		data_prefix;
    bool			realaliases_;

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

protected:

    bool		realaliases_;
    BufferString	rshcomm_;
    int			defnicelvl_;
    int			portnr_;

    void		handleLocal();
    bool		readHostFile(const char*);

};


#endif
