#ifndef changetracker_h
#define changetracker_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: changetracker.h,v 1.1 2001-09-26 12:13:42 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include <string.h>

/*!\brief updates a variable when changes occur.

Use if you need to keep track of whether a variable changes when it is assigned
to another variable. Example: a 'not saved' flag in a UI.

*/


class ChangeTracker
{
public:
			ChangeTracker( bool& c )
			: chgd(c)			{}

    template <class T>
    inline bool		update(T& val,const T& newval);
			//!< returns whterh this value is changed
    inline bool		update(char*&,const char*&);
			//!< specialisation for C-strings

    void		setChanged( bool yn=true )	{ chgd = yn; }

protected:

    bool		chgd;

};


template <class T>
inline bool ChangeTracker::updateVal( T& val, const T& newval )
{
    bool ret = newval == val;
    val = newval;
    chgd = chgd || !ret;
    return ret;
}


inline bool ChangeTracker::update( char*& val, const char*& newval )
{
    bool ret = (val && newval) || (!val && !newval);
    if ( !ret ) { chgd = true; return true; }
    if ( !val ) return false;

    ret = strcmp( val, newval );
    chgd = chgd || !ret;
    return ret;
}


#endif
