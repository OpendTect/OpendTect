#ifndef hostdata_h
#define hostdata_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.h,v 1.1 2002-04-05 16:29:57 bert Exp $
________________________________________________________________________

-*/

#include <sets.h>

/*\brief Host name and other data */

class HostData
{
public:

    				HostData( const char* nm )
				: name_(nm)	{}
    virtual			~HostData()	{ deepErase(aliases_); }

    const char*			name() const	{ return officialName(); }
    const char*			officialName() const
				{ return (const char*)name_; }
    const char*			shortestName() const;
    int				nrAliases() const
				{ return aliases_.size(); }
    const char*			alias( int idx ) const
				{ return (const char*)(*aliases_[idx]); }

protected:

    BufferString		name_;
    ObjectSet<BufferString>	aliases_;

    friend class		HostDataList;

};


/*\brief List of host names in the system */

class HostDataList : public ObjectSet<HostData>
{
public:
			HostDataList();
    virtual		~HostDataList()	{ deepErase(*this); }

};


#endif
