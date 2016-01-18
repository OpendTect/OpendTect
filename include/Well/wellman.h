#ifndef wellman_h
#define wellman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "sets.h"
#include "bufstring.h"

class IOObj;
class MultiID;
class BufferStringSet;

namespace Well
{

class Data;

/*!
\brief Well manager
*/

mExpClass(Well) Man
{
public:
			~Man();

    void		removeObject( const Well::Data* );
    Data*		get(const MultiID&);
    void		add(const MultiID&,Data*);
			//!< Data becomes mine
    Data*		release(const MultiID&);
			//!< Data becomes yours
    bool		isLoaded(const MultiID&) const;
    bool		reload(const MultiID&);

    const char*		errMsg() const		{ return msg_; }
    ObjectSet<Data>&	wells()			{ return wells_; }

    static bool		getLogNames(const MultiID&,BufferStringSet&);
    static bool		getMarkerNames(BufferStringSet&);

protected:

			Man()				{}
    static Man*		mgr_;
    mGlobal(Well) friend Man&	MGR();

    ObjectSet<Data>	wells_;
    BufferString	msg_;

    int			gtByKey(const MultiID&) const;
};

mGlobal(Well) Man& MGR();

mGlobal(Well) IOObj* findIOObj(const char* wellnm,const char* uwi);

} // namespace Well

#endif
