#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/09/2000
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"

/*!
\brief Updates a variable when changes occur.

  Use if you need to keep track of whether a variable changes when it is
  assigned to another variable. Example: a 'not saved' flag in a UI. Also
  facilitates giving unique change stamps.
*/

mClass(Algo) ChangeTracker
{
public:
			ChangeTracker( bool* c=0 )
			: chgd_(c), chgid_(0)		{}
			ChangeTracker( bool& c )
			: chgd_(&c), chgid_(0)		{}
			ChangeTracker( unsigned int& c )
			: chgid_(&c), chgd_(0)		{}
			ChangeTracker( unsigned int* c )
			: chgid_(c), chgd_(0)		{}
			ChangeTracker( bool& c, unsigned int& ci )
			: chgd_(&c), chgid_(&ci)	{}

			//! returns wether this value is changed
    template <class T,class U>
    inline bool		set(const T& oldval,const U& newval);
			//! Changes and returns wether this value is changed
    template <class T,class U>
    inline bool		update(T& val,const U& newval);

			//! specialization for C-strings
    inline bool		set(const char*&,const char*&);

    bool		isChanged() const
			{ return chgd_ ? *chgd_ : (bool)(chgid_ ? *chgid_ : 0);}
    unsigned int	changeId() const
			{ return chgid_ ? *chgid_ : (chgd_ ? (*chgd_?1:0) : 0);}
    inline void		setChanged(bool yn=true);
    void		setChangeId( unsigned int c )
			{ if ( chgid_ ) *chgid_ = c; }

    bool		hasBoolVar() const		{ return chgd_; }
    bool		hasIntVar() const		{ return chgid_; }
    const bool&		boolVar() const			{ return *chgd_; }
			//!< Don't call if !hasBoolVar()
    const unsigned int&	intVar() const			{ return *chgid_; }
			//!< Don't call if !hasIntVar()

    void		setVar( bool* m )		{ chgd_ = m; }
    void		setVar( bool& m )		{ chgd_ = &m; }
    void		setVar( unsigned int* m )	{ chgid_ = m; }
    void		setVar( unsigned int& m )	{ chgid_ = &m; }

protected:

    bool*		chgd_;
    unsigned int*	chgid_;

};


/*!
\ingroup Algo
\brief Macro to use when there is no direct access to data members.

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
    if ( chgd_ )
	{ if ( !*chgd_ ) *chgd_ = ischgd; }
    else if ( chgid_ )
	{ if ( ischgd ) (*chgid_)++; }
}


template <class T,class U>
inline bool ChangeTracker::set( const T& val, const U& newval )
{
    bool ret = !(val ==  mCast(T,newval));
    setChanged( ret );
    return ret;
}


inline bool ChangeTracker::set( const char*& val, const char*& newval )
{
    bool ret = (val && newval) || (!val && !newval);
    if ( !ret ) { setChanged(true); return true; }
    if ( !val ) return false;

    ret = StringView(val)!=newval;
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


