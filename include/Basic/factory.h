#ifndef factory_h
#define factory_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Sep 1994, Aug 2006
 RCS:		$Id: factory.h,v 1.1 2006-11-14 19:32:56 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"

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
    			{ thefactory.addCreator(createFunc,"My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactory( ClassName, FunctionName );
\endcode

that will create a static function that returns an instance to
Factory<ClassName>. The static function must be implemented in a src-file
with the macro

\code
mImplFactory( ClassName, FunctionName );
\endcode

*/


template <class T>
class Factory
{
public:
    typedef			T* (*Creator)();
    inline void			addCreator(Creator,const char* nm);
    inline T*			create(const char* nm) const;
    const BufferStringSet&	getNames() const { return names_; }
protected:

    BufferStringSet		names_;
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
    			{ thefactory.addCreator(createFunc,"My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactoryWithParam( ClassName, ParamClass, FunctionName );
\endcode

that will create a static function that returns an instance to
FactoryWithParam<ClassName,ParamClass>. The static function must be implemented
in a src-file with the macro

\code
mImplFactoryWithParam( ClassName, ParamClass, FunctionName );
\endcode

*/


template <class T, class P>
class FactoryWithParam
{
public:
    typedef			T* (*Creator)(P);
    inline void			addCreator(Creator,const char* nm);
    inline T*			create(const char* nm, P) const;
    const BufferStringSet&	getNames() const { return names_; }
protected:

    BufferStringSet		names_;
    TypeSet<Creator>		creators_;
};


template <class T> inline
void Factory<T>::addCreator( Creator cr, const char* name )
{
    names_.add( name );
    creators_ += cr;
}


template <class T> inline
T* Factory<T>::create( const char* name ) const
{
    const int idx = names_.indexOf( name );
    if ( idx<0 ) return 0;

    return creators_[idx]();
}


template <class T, class P> inline
void FactoryWithParam<T,P>::addCreator( Creator cr, const char* name )
{
    names_.add( name );
    creators_ += cr;
}


template <class T, class P> inline
T* FactoryWithParam<T,P>::create( const char* name, P data ) const
{
    const int idx = names_.indexOf( name );
    if ( idx<0 ) return 0;

    return creators_[idx]( data );
}


#define mDefineFactory( T, funcname ) \
Factory<T>& funcname()


#define mImplFactory( T, funcname ) \
Factory<T>& funcname() \
{ \
    static Factory<T> inst; \
    return inst; \
} 


#define mDefineFactoryWithParam( T, P, funcname ) \
FactoryWithParam<T,P>& funcname()


#define mImplFactoryWithParam( T, P, funcname ) \
FactoryWithParam<T,P>& funcname() \
{ \
    static FactoryWithParam<T,P> inst; \
    return inst; \
} 
#endif
