#ifndef factory_h
#define factory_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Sep 1994, Aug 2006
 RCS:		$Id: factory.h,v 1.8 2007-12-03 22:10:31 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "errh.h"

/*!
Generalized static factory that can deliver instances of T, when no
variable is needed in the creation.

Usage. Each implementation of the base class T must add themselves
to the factory when application starts up, e.g. in an initClass() function:
\code
class A
{
public:
    virtual int		myFunc() 	= 0;
};

class B : public A
{
public:
    static A*		createFunc() { return new B; }
    static void		initClass()
    			{ thefactory.addCreator(createFunc,"MyKeyword",
						"My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactory( ClassName, FunctionName );
\endcode

that will create a static function that returns an instance to
Factory<ClassName>.
If the function is a static member of a class, it has to be defined with
the mDefineFactoryInClass macro.

The static function must be implemented in a src-file with the macro

\code
mImplFactory( ClassName, FunctionName );
\endcode

*/


template <class T>
class Factory
{
public:
    typedef			T* (*Creator)();
    inline void			addCreator(Creator,const char* nm,
	    				   const char* username = 0);
    				//!<Name may be not be null
    inline T*			create(const char* nm) const;
    				//!<Name may be not be null
    const BufferStringSet&	getNames(bool username=false) const;
protected:

    BufferStringSet		names_;
    BufferStringSet		usernames_;
    TypeSet<Creator>		creators_;
};


/*!
Generalized static factory that can deliver instances of T, when a
variable is needed in the creation.

Usage. Each implementation of the base class T must add themselves
to the factory when application starts up, e.g. in an initClass() function:
\code
class A
{
public:
    virtual int		myFunc() 	= 0;
};

class B : public A
{
public:
    static A*		createFunc(C* param) { return new B(param); }
    static void		initClass()
    			{ thefactory.addCreator(createFunc,"MyKeyword","My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactory1Param( ClassName, ParamClass, FunctionName );
\endcode

that will create a static function that returns an instance to
Factory1Param<ClassName,ParamClass>. The static function must be implemented
in a src-file with the macro

\code
mImplFactory1Param( ClassName, ParamClass, FunctionName );
\endcode

*/


template <class T, class P>
class Factory1Param
{
public:
    typedef			T* (*Creator)(P);
    inline void			addCreator(Creator,const char* nm=0,
	    				   const char* usernm = 0);
    				//!<Name may be be null
    inline T*			create(const char* nm, P, bool chknm=true)const;
    				//!<Name may be be null, if null name is given
				//!<chknm will be forced to false
    const BufferStringSet&	getNames(bool username=false) const;
protected:

    BufferStringSet		names_;
    BufferStringSet		usernames_;
    TypeSet<Creator>		creators_;
};


template <class T, class P0, class P1>
class Factory2Param
{
public:
    typedef			T* (*Creator)(P0,P1);
    inline void			addCreator(Creator,const char* nm=0,
	    				   const char* usernm = 0);
    				//!<Name may be be null
    inline T*			create(const char* nm, P0, P1,
	    			       bool chknm=true)const;
    				//!<Name may be be null, if null name is given
				//!<chknm will be forced to false
    const BufferStringSet&	getNames(bool username=false) const;
protected:

    BufferStringSet		names_;
    BufferStringSet		usernames_;
    TypeSet<Creator>		creators_;
};


template <class T> inline
void Factory<T>::addCreator( Creator cr, const char* name,
			     const char* username )
{
    if ( !name )
	return;

    const int idx = names_.indexOf( name );
    if ( idx!=-1 )
    {
	if ( (username && *usernames_[idx]!=username) ||
	     creators_[idx]!=cr )
	{
	    BufferString msg = "Refusing to add factory ";
	    msg += name;
	    pErrMsg(msg.buf());
	}

	return;
    }

    names_.add( name );
    usernames_.add( username ? username : name );
    creators_ += cr;
}


template <class T> inline
T* Factory<T>::create( const char* name ) const
{
    if ( !name )
	return 0;

    const int idx = names_.indexOf( name );
    if ( idx<0 ) return 0;

    return creators_[idx]();
}


template <class T> inline
const BufferStringSet& Factory<T>::getNames( bool username ) const
{ return username ? usernames_ : names_; }


template <class T, class P> inline
void Factory1Param<T,P>::addCreator( Creator cr, const char* name,
				     const char* username )
{
    names_.add( name );
    usernames_.add( username ? username : name );

    creators_ += cr;
}


template <class T, class P> inline
T* Factory1Param<T,P>::create( const char* name, P data, bool chk ) const
{
    if ( chk )
    {
	const int idx = names_.indexOf( name );
	if ( idx<0 ) return 0;

	return creators_[idx]( data );
    }

    for ( int idx=0; idx<creators_.size(); idx++ )
    {
	T* res = creators_[idx]( data );
	if ( res ) return res;
    }

    return 0;
}


template <class T, class P> inline
const BufferStringSet& Factory1Param<T,P>::getNames( bool username ) const
{ return username ? usernames_ : names_; }


template <class T, class P0, class P1> inline
void Factory2Param<T,P0,P1>::addCreator( Creator cr, const char* name,
					 const char* username )
{
    names_.add( name );
    usernames_.add( username ? username : name );
    creators_ += cr;
}


template <class T, class P0, class P1> inline
T* Factory2Param<T,P0,P1>::create( const char* name, P0 p0, P1 p1,
				   bool chk ) const
{
    if ( chk )
    {
	const int idx = names_.indexOf( name );
	if ( idx<0 ) return 0;

	return creators_[idx]( p0,p1 );
    }

    for ( int idx=0; idx<creators_.size(); idx++ )
    {
	T* res = creators_[idx]( p0, p1 );
	if ( res ) return res;
    }

    return 0;
}


template <class T, class P0, class P1> inline
const BufferStringSet& Factory2Param<T,P0,P1>::getNames( bool username ) const
{ return username ? usernames_ : names_; }


#define mDefineFactory( T, funcname ) \
::Factory<T>& funcname()


#define mDefineFactoryInClass( T, funcname ) \
static ::Factory<T>& funcname()


#define mImplFactory( T, funcname ) \
::Factory<T>& funcname() \
{ \
    static ::Factory<T> inst; \
    return inst; \
} 


#define mDefineFactory1Param( T, P, funcname ) \
::Factory1Param<T,P>& funcname()


#define mDefineFactory1ParamInClass( T, P, funcname ) \
static ::Factory1Param<T,P>& funcname()


#define mImplFactory1Param( T, P, funcname ) \
::Factory1Param<T,P>& funcname() \
{ \
    static ::Factory1Param<T,P> inst; \
    return inst; \
} 


#define mDefineFactory2Param( T, P0, P1, funcname ) \
::Factory2Param<T,P0,P1>& funcname()


#define mDefineFactory2ParamInClass( T, P0, P1, funcname ) \
static ::Factory2Param<T,P0,P1>& funcname()


#define mImplFactory2Param( T, P0, P1, funcname ) \
::Factory2Param<T,P0,P1>& funcname() \
{ \
    static ::Factory2Param<T,P0,P1> inst; \
    return inst; \
} 
#endif
