#ifndef wellman_h
#define wellman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellman.h,v 1.9 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"

class MultiID;

namespace Well
{

class Data;

mClass Man
{
public:

    			~Man();

    Data*		get(const MultiID&,bool force_reload=false);
    void		add(const MultiID&,Data*);
    			//!< Data becomes mine
    Data*		release(const MultiID&);
    			//!< Data becomes yours
    bool		isLoaded(const MultiID&) const;
    bool		reload(const MultiID&);

    const char*		errMsg() const		{ return msg_; }
    ObjectSet<Data>&	wells()			{ return wells_; }
    ObjectSet<MultiID>&	keys()			{ return keys_; }

protected:

			Man()				{}
    static Man*		mgr_;
    mGlobal friend Man&	MGR();

    ObjectSet<Data>	wells_;
    ObjectSet<MultiID>	keys_;
    BufferString	msg_;

};

mGlobal Man& MGR();

}; // namespace Well




#endif
