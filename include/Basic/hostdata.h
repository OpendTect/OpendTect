#ifndef hostdata_h
#define hostdata_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.h,v 1.3 2002-05-13 14:34:59 bert Exp $
________________________________________________________________________

-*/

#include <sets.h>

/*\brief Host name and aliases */

class HostData
{
public:

    				HostData( const char* nm )
				: name_(nm)	{}
    virtual			~HostData()	{ deepErase(aliases_); }

    const char*			name() const;
    const char*			officialName() const
				{ return (const char*)name_; }
    const char*			shortestName() const;

    int				nrAliases() const
				{ return aliases_.size(); }
    const char*			alias( int idx ) const
				{ return (const char*)(*aliases_[idx]); }
    bool			isKnownAs(const char*) const;
    				//!< true if name or an alias matches
    void			addAlias(const char*);
    				//!< only adds if !isKnownAs

    static const char*		localHostName();

protected:

    BufferString		name_;
    ObjectSet<BufferString>	aliases_;

    friend class		HostDataList;

};


/*\brief List of host names in the system

  The first entry should be the local host.
 
 */

class HostDataList : public ObjectSet<HostData>
{
public:
			HostDataList();
    virtual		~HostDataList()	{ deepErase(*this); }

    const char*		optimumName( int idx )
			{ return idx ? (*this)[idx]->shortestName()
			    	     : (*this)[idx]->officialName(); }

private:

    void		handleLocal();

};


#endif
