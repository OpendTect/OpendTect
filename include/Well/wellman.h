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

class MultiID;

namespace Well
{

class Data;

mExpClass(Well) Man
{
public:

    			~Man();

    void		removeAll();

    Data*		get(const MultiID&,bool force_reload=false);
    void		add(const MultiID&,Data*);
    			//!< Data becomes mine
    Data*		release(const MultiID&);
    			//!< Data becomes yours
    bool		isLoaded(const MultiID&) const;
    bool		reload(const MultiID&);

    const char*		errMsg() const		{ return msg_; }
    ObjectSet<Data>&	wells()			{ return wells_; }

protected:

			Man()				{}
    static Man*		mgr_;
    mGlobal(Well) friend Man&	MGR();

    ObjectSet<Data>	wells_;
    BufferString	msg_;

    int			gtByKey(const MultiID&) const;
};

mGlobal(Well) Man& MGR();

}; // namespace Well




#endif

