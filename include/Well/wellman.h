#ifndef wellman_h
#define wellman_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellman.h,v 1.1 2003-08-27 10:19:39 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"

class MultiID;

namespace Well
{

class Data;

class Man
{
public:


    Data*		get(const MultiID&,bool force_reload=false);
    const char*		errMsg() const		{ return msg_; }
    ObjectSet<Data>&	wells()			{ return wells_; }
    ObjectSet<MultiID>&	keys()			{ return keys_; }

protected:

			Man()				{}
    static Man*		mgr_;
    friend Man&		MGR();

    ObjectSet<Data>	wells_;
    ObjectSet<MultiID>	keys_;
    BufferString	msg_;

};

}; // namespace Well


inline Well::Man& Well::MGR()
{
    if ( !Well::Man::mgr_ ) Well::Man::mgr_ = new Well::Man;
    return *Well::Man::mgr_;
}


#endif
