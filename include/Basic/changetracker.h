#ifndef changetracker_h
#define changetracker_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          26/09/2000
 RCS:           $Id: changetracker.h,v 1.3 2001-09-27 10:32:03 nanne Exp $
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
			ChangeTracker( bool* c=0 )
			: chgd(c)			{}
			ChangeTracker( bool& c )
			: chgd(&c)			{}

    template <class T,class U>
    inline bool		update(T& val,const U& newval);
			//!< returns whterh this value is changed
    inline bool		update(char*&,const char*&);
			//!< specialisation for C-strings

    bool		isChanged() const	   { return chgd && *chgd;}
    void		setChanged( bool yn=true ) { if ( chgd ) *chgd = yn; }

    bool		hasVar() const			{ return chgd; }
    const bool&		var() const			{ return *chgd; }
			//!< Don't call if !hasVar()
    void		setVar( bool* m )		{ chgd = m; }
    void		setVar( bool& m )		{ chgd = &m; }

protected:

    bool*		chgd;

};


/*!\brief macro to use when there is no direct access to data members.

chtr = the change tracker
obj = object instance
getfn = get function
setfn - set function
newval = new value
*/


#define mChgTrackGetSet(chtr,obj,getfn,setfn,newval) { \
    if ( !chtr.isChanged() && newval != obj->getfn() ) \
	chtr.setChanged( true ); \
    obj->setfn( newval ); }


template <class T,class U>
inline bool ChangeTracker::update( T& val, const U& newval )
{
    if ( !chgd ) return false;
    bool ret = !(newval == val);
    val = newval;
    if ( !*chgd ) *chgd = ret;
    return ret;
}


inline bool ChangeTracker::update( char*& val, const char*& newval )
{
    if ( !chgd ) return false;
    bool ret = (val && newval) || (!val && !newval);
    if ( !ret ) { *chgd = true; return true; }
    if ( !val ) return false;

    ret = strcmp( val, newval );
    if ( !*chgd ) *chgd = ret;
    return ret;
}


#endif
