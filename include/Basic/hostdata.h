#ifndef hostdata_h
#define hostdata_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.h,v 1.6 2002-12-18 12:15:58 arend Exp $
________________________________________________________________________

-*/

#include <sets.h>

/*\brief Host name and aliases */

class HostData
{
public:

    				HostData( const char* nm, bool ra=true )
				: name_(nm), realaliases_(ra), status(0) {}
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

    static const char*		localHostName();

    int				status; 

protected:

    BufferString		name_;
    ObjectSet<BufferString>	aliases_;
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
    const char*		rshComm() const		{ return rshcomm_; }

protected:

    bool		realaliases_;
    BufferString	rshcomm_;
    int			defnicelvl_;

    void		handleLocal();
    bool		readHostFile(const char*);

};


#endif
