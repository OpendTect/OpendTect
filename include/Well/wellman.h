#ifndef wellman_h
#define wellman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellman.h,v 1.10 2012-05-31 13:17:35 cvsbruno Exp $
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
    mGlobal friend Man&	MGR();

    ObjectSet<Data>	wells_;
    BufferString	msg_;

    int			gtByKey(const MultiID&) const;
};

mGlobal Man& MGR();

}; // namespace Well




#endif
