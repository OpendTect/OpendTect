#ifndef changetracker_h
#define changetracker_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/09/2000
 RCS:           $Id: changetracker.h,v 1.10 2009/07/22 16:01:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include <string.h>

/*!\brief updates a variable when changes occur.

Use if you need to keep track of whether a variable changes when it is assigned
to another variable. Example: a 'not saved' flag in a UI. Also facilitates
Giving unique change stamps.

*/


mClass ChangeTracker
{
public:
			ChangeTracker( bool* c=0 )
			: chgd(c), chgid(0)		{}
			ChangeTracker( bool& c )
			: chgd(&c), chgid(0)		{}
			ChangeTracker( unsigned int& c )
			: chgid(&c), chgd(0)		{}
			ChangeTracker( unsigned int* c )
			: chgid(c), chgd(0)		{}
			ChangeTracker( bool& c, unsigned int& ci )
			: chgd(&c), chgid(&ci)		{}

			//! returns wether this value is changed
    template <class T,class U>
    inline bool		set(const T& oldval,const U& newval);
			//! Changes and returns wether this value is changed
    template <class T,class U>
    inline bool		update(T& val,const U& newval);

			//! specialisation for C-strings
    inline bool		set(const char*&,const char*&);
			//! specialisation for C-strings
    inline bool		update(char*&,const char*&);

    bool		isChanged() const
    			{ return chgd ? *chgd : (bool)(chgid ? *chgid : 0); }
    unsigned int	changeId() const
    			{ return chgid ? *chgid : (chgd ? (*chgd?1:0) : 0); }
    inline void		setChanged(bool yn=true);
    void		setChangeId( unsigned int c )
    			{ if ( chgid ) *chgid = c; }

    bool		hasBoolVar() const		{ return chgd; }
    bool		hasIntVar() const		{ return chgid; }
    const bool&		boolVar() const			{ return *chgd; }
			//!< Don't call if !hasBoolVar()
    const unsigned int&	intVar() const			{ return *chgid; }
			//!< Don't call if !hasIntVar()

    void		setVar( bool* m )		{ chgd = m; }
    void		setVar( bool& m )		{ chgd = &m; }
    void		setVar( unsigned int* m )	{ chgid = m; }
    void		setVar( unsigned int& m )	{ chgid = &m; }

protected:

    bool*		chgd;
    unsigned int*	chgid;

};


/*!\brief macro to use when there is no direct access to data members.

chtr = the change tracker
obj = object instance
getfn = get function
setfn - set function
newval = new value
*/


#define mChgTrackGetSet(chtr,obj,getfn,setfn,newval) { \
    if ( chtr.set( obj->getfn(), newval ) ) \
	obj->setfn( newval ); }


inline void ChangeTracker::setChanged( bool ischgd )
{
    if ( chgd )
	{ if ( !*chgd ) *chgd = ischgd; }
    else if ( chgid )
	{ if ( ischgd ) (*chgid)++; }
}


template <class T,class U>
inline bool ChangeTracker::set( const T& val, const U& newval )
{
    bool ret = !(val == newval);
    setChanged( ret );
    return ret;
}


inline bool ChangeTracker::set( const char*& val, const char*& newval )
{
    bool ret = (val && newval) || (!val && !newval);
    if ( !ret ) { setChanged(true); return true; }
    if ( !val ) return false;

    ret = strcmp( val, newval );
    setChanged( ret );
    return ret;
}


template <class T,class U>
inline bool ChangeTracker::update( T& val, const U& newval )
{
    bool ret = set( val, newval );
    val = newval;
    return ret;
}


inline bool ChangeTracker::update( char*& val, const char*& newval )
{
    bool ret = set( *((const char**)(&val)), newval );
    if ( val ) strcpy( val, newval ? newval : "" );
    return ret;
}


#endif
