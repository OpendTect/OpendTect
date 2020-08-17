#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Sep 1994, Aug 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "typeset.h"
#include "uistring.h"

/*!
\brief Base class for Factories ( Factory, Factory1Param, Factory2Param
and Factory3Param. )
*/

mExpClass(Basic) FactoryBase
{
public:
    virtual			~FactoryBase();

    int				size() const;
    bool			isEmpty() const;

    bool			hasName(const char* n) {return indexOf(n)>=0;}
    const BufferStringSet&	getNames() const;
    const uiStringSet&		getUserNames() const;
    void			setDefaultName(int idx);
				//!<idx refers to names in names_,
				//!<or -1 for none
    const char*			getDefaultName() const;
    static char			cSeparator()	{ return ','; }

    BufferString&		errMsg() const;
				//!<Threadsafe, as each thread will have
				//!<a different string returned.

    const char*			currentName() const;
				/*!<Is set only when calling the create-
				    functions, so they can know what was
				    requested.
				    \note Threadsafe, as each thread will have
					  a different string returned.
				 */

protected:

    int				indexOf(const char*) const;
    void			addNames(const char*,const uiString&);
    void			setNames(int,const char*,const uiString&);

    mutable StaticStringManager	errmsgs_;
    mutable StaticStringManager currentname_;

private:

    BufferStringSet		names_;
    uiStringSet			usernames_;
    BufferStringSet		aliases_;
    BufferString		defaultname_;
};


/*!
\brief Generalized static factory that can deliver instances of T, when no
variable is needed in the creation.

  Usage. Each implementation of the base class T must add themselves
  to the factory when application starts up, e.g. in an initClass() function:
  \code
  class A
  {
  public:
  virtual int		myFunc()	= 0;
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
  mDefineFactory( Module, ClassName, FunctionName );
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
mClass(Basic) Factory : public FactoryBase
{ mODTextTranslationClass(Factory)
public:
    typedef			T* (*Creator)();
    inline void			addCreator(Creator,const char* nm,
					const uiString& username =
					   uiString::emptyString());
				/*!<Name may be not be null
				   If nm is found, old creator is replaced.
				   nm can be a SeparString, separated by
				   cSeparator(), allowing multiple names,
				   where the first name will be the main
				   name that is returned in getNames. */

    inline T*			create(const char* nm) const;
				//!<Name may be not be null
protected:

    TypeSet<Creator>		creators_;
};


/*!
\brief Generalized static factory that can deliver instances of T, when a
variable is needed in the creation.

  Usage. Each implementation of the base class T must add themselves
  to the factory when application starts up, e.g. in an initClass() function:
  \code
  class A
  {
  public:
  virtual int		myFunc()	= 0;
  };

  class B : public A
  {
  public:
  static A*		createFunc(C* param)
  {
  A* res = new B;
  if ( res->setParam( param ) );
  return res;

  thefactory.errMsg() = "Could not set param";
  delete res;
  return 0;
  }
  static void		initClass()
  { thefactory.addCreator(createFunc,"MyKeyword","My Name"); }

  int			myFunc();
  };

  \endcode

  Two macros are available to make a static accessfuncion for the factory:
  \code
  mDefineFactory1Param( Module, ClassName, ParamExpClass, FunctionName );
  \endcode

  that will create a static function that returns an instance to
  Factory1Param<ClassName,ParamExpClass>. The static function must be
  implemented in a src-file with the macro.

  \code
  mImplFactory1Param( ClassName, ParamExpClass, FunctionName );
  \endcode
*/

template <class T, class P>
mClass(Basic) Factory1Param : public FactoryBase
{ mODTextTranslationClass(Factory1Param)
public:
    typedef			T* (*Creator)(P);
    inline void			addCreator(Creator,const char* nm=nullptr,
					   const uiString& usernm =
						uiString::emptyString());
				/*!<Name may be be null
				   If nm is found, old creator is replaced.
				   nm can be a SeparString, separated by
				   cSeparator(), allowing multiple names,
				   where the first name will be the main
				   name that is returned in getNames. */
    inline T*			create(const char* nm, P, bool chknm=true)const;
				//!<Name may be be null, if null name is given
				//!<chknm will be forced to false
protected:

    TypeSet<Creator>		creators_;
};


/*!
\brief Subclass of FactoryBase.
*/

template <class T, class P0, class P1>
mClass(Basic) Factory2Param : public FactoryBase
{
public:
    typedef		T* (*Creator)(P0,P1);
    inline void		addCreator(Creator,const char* nm=nullptr,
				const uiString& usernm=uiString::emptyString());
			/*!<Name may be be null
			   If nm is found, old creator is replaced.
			   nm can be a SeparString, separated by
			   cSeparator(), allowing multiple names,
			   where the first name will be the main
			   name that is returned in getNames. */
    inline T*		create(const char* nm, P0, P1,
			       bool chknm=true)const;
			//!<Name may be be null, if null name is given
			//!<chknm will be forced to false
protected:

    TypeSet<Creator>		creators_;
};


/*!
\brief Subclass of FactoryBase.
*/

template <class T, class P0, class P1, class P2>
mClass(Basic) Factory3Param : public FactoryBase
{
public:
    typedef		T* (*Creator)(P0,P1,P2);
    inline void		addCreator(Creator,const char* nm=nullptr,
			    const uiString& usernm =uiString::emptyString());
			/*!<Name may be be null
			   If nm is found, old creator is replaced.
			   nm can be a SeparString, separated by
			   cSeparator(), allowing multiple names,
			   where the first name will be the main
			   name that is returned in getNames. */
    inline T*		create(const char* nm, P0, P1, P2,
			       bool chknm=true)const;
			//!<Name may be be null, if null name is given
			//!<chknm will be forced to false
protected:

    TypeSet<Creator>		creators_;
};


#define mDefaultFactoryStringImpl \
    const char*		factoryKeyword() const { return sFactoryKeyword(); } \
    uiString		factoryDisplayName() const \
					{ return sFactoryDisplayName(); }

#define mDefaultStaticFactoryStringDeclaration \
    static const char*	sFactoryKeyword(); \
    static uiString	sFactoryDisplayName()

#define mDefaultFactoryInitClassImpl( baseclss, createfunc ) \
{ \
    baseclss::factory().addCreator(createfunc,sFactoryKeyword(), \
				   sFactoryDisplayName()); \
}

#define mDefaultFactoryInstanciationBase( keywrd, usernm ) \
    mDefaultFactoryStringImpl \
    static const char*	sFactoryKeyword() { return keywrd; } \
    static uiString 	sFactoryDisplayName() { return usernm; } \
    static void		initClass()

#define mDefaultFactoryCreatorImpl( baseclss, clss ) \
static baseclss*	createInstance() { return new clss; } \

#define mDefaultFactoryInstantiation( baseclss, clss, keywrd, usernm ) \
    mDefaultFactoryCreatorImpl( baseclss, clss ); \
    mDefaultFactoryInstanciationBase( keywrd, usernm ) \
    mDefaultFactoryInitClassImpl( baseclss, createInstance )


#define mDefaultFactoryCreatorImpl1Param( baseclss, clss, parclss ) \
static baseclss*	createInstance( parclss p1 ) \
			{ return new clss( p1 ); }

#define mDefaultFactoryInstantiation1Param( baseclss, clss, parclss,\
					    keywrd, usernm ) \
    mDefaultFactoryCreatorImpl1Param( baseclss, clss, parclss ) \
    mDefaultFactoryInstanciationBase( keywrd, usernm ) \
    mDefaultFactoryInitClassImpl( baseclss, createInstance )

#define mDefaultFactoryCreatorImpl2Param( baseclss, clss, parclss1, parclss2 ) \
static baseclss*	createInstance( parclss1 p1, parclss2 p2 ) \
			{ return new clss( p1, p2 ); }

#define mDefaultFactoryInstantiation2Param( baseclss, clss, parclss1,\
					    parclss2, keywrd, usernm ) \
    mDefaultFactoryCreatorImpl2Param( baseclss, clss, parclss1, parclss2 ) \
    mDefaultFactoryInstanciationBase( keywrd, usernm ) \
    mDefaultFactoryInitClassImpl( baseclss, createInstance )


#define mCreateImpl( donames, createfunc ) \
    currentname_.getObject() = name; \
    if ( donames ) \
    { \
	const int idx = indexOf( name ); \
	if ( idx==-1 ) \
	{ \
	    errmsgs_.getObject() = "Name "; \
	    errmsgs_.getObject().add( name ).add(" not found.\n" ) \
				.add( "Perhaps all plugins are not loaded\n" );\
	    return 0; \
	} \
	return createfunc; \
    } \
 \
    for ( int idx=0; idx<creators_.size(); idx++ ) \
    { \
	T* res = createfunc; \
	if ( res ) return res; \
    } \
 \
    return 0


#define mAddCreator \
    const int idx = indexOf( name );\
\
    if ( idx==-1 )\
    { \
	addNames( name, username ); \
	creators_ += cr; \
    } \
    else\
    {\
	setNames( idx, name, username ); \
	creators_[idx] = cr;\
    }

template <class T> inline
void Factory<T>::addCreator( Creator cr, const char* name,
			     const uiString& username )
{
    if ( !name ) return;\
    mAddCreator;
}


template <class T> inline
T* Factory<T>::create( const char* name ) const
{
    mCreateImpl( true, creators_[idx]() );
}


template <class T, class P> inline
void Factory1Param<T,P>::addCreator( Creator cr, const char* name,
				     const uiString& username )
{
    mAddCreator;
}


template <class T, class P> inline
T* Factory1Param<T,P>::create( const char* name, P data, bool chk ) const
{
    mCreateImpl( chk, creators_[idx]( data ) );
}


template <class T, class P0, class P1> inline
void Factory2Param<T,P0,P1>::addCreator( Creator cr, const char* name,
					 const uiString& username )
{
    mAddCreator;
}


template <class T, class P0, class P1> inline
T* Factory2Param<T,P0,P1>::create( const char* name, P0 p0, P1 p1,
				   bool chk ) const
{
    mCreateImpl( chk, creators_[idx]( p0, p1 ) );
}


template <class T, class P0, class P1, class P2> inline
void Factory3Param<T,P0,P1,P2>::addCreator( Creator cr, const char* name,
					 const uiString& username )
{
    mAddCreator;
}


template <class T, class P0, class P1, class P2> inline
T* Factory3Param<T,P0,P1,P2>::create( const char* name, P0 p0, P1 p1, P2 p2,
				   bool chk ) const
{
    mCreateImpl( chk, creators_[idx]( p0, p1, p2 ) );
}


#define mDefineFactory( mod, T, funcname ) \
mGlobal(mod) ::Factory<T>& funcname()


#define mDefineFactoryInClasswKW( T, funcname, kw ) \
static ::Factory<T>& funcname(); \
virtual uiString factoryDisplayName() const \
{ return ::toUiString( factoryKeyword() ); } \
virtual const char* factoryKeyword() const { return kw; }
#define mDefineFactoryInClass( T, funcname ) \
    mDefineFactoryInClasswKW( T, funcname, nullptr )


#define mImplFactory( T, funcname ) \
::Factory<T>& funcname() \
{ \
    mDefineStaticLocalObject(PtrMan< ::Factory<T> >,inst,(new ::Factory<T>)); \
    return *inst; \
}


#define mDefineFactory1Param( mod, T, P, funcname ) \
mGlobal(mod) ::Factory1Param<T,P>& funcname()


#define mDefineFactory1ParamInClasswKW( T, P, funcname, kw ) \
static ::Factory1Param<T,P>& funcname(); \
virtual uiString factoryDisplayName() const \
{ return ::toUiString(factoryKeyword()); } \
virtual const char* factoryKeyword() const { return kw; }
#define mDefineFactory1ParamInClass( T, P, funcname ) \
    mDefineFactory1ParamInClasswKW( T, P, funcname, nullptr )

#define mImplFactory1Param( T, P, funcname ) \
::Factory1Param<T,P>& funcname() \
{ \
    mLockStaticInitLock( static_inst_lck__ ); \
    static PtrMan< ::Factory1Param<T,P> > \
	inst(new ::Factory1Param<T,P>); \
    mUnlockStaticInitLock( static_inst_lck__ ); \
     \
    return *inst; \
}


#define mDefineFactory2Param( mod, T, P0, P1, funcname ) \
mGlobal(mod) ::Factory2Param<T,P0,P1>& funcname()


#define mDefineFactory2ParamInClasswKW( T, P0, P1, funcname, kw ) \
static ::Factory2Param<T,P0,P1>& funcname(); \
virtual uiString factoryDisplayName() const \
{ return ::toUiString( factoryKeyword() ); } \
virtual const char* factoryKeyword() const { return kw; }
#define mDefineFactory2ParamInClass( T, P0, P1, funcname ) \
    mDefineFactory2ParamInClasswKW( T, P0, P1, funcname, nullptr )


#define mImplFactory2Param( T, P0, P1, funcname ) \
::Factory2Param<T,P0,P1>& funcname() \
{ \
    mLockStaticInitLock( static_inst_lck__ ); \
    static PtrMan< ::Factory2Param<T,P0,P1> > \
	inst(new ::Factory2Param<T,P0,P1>); \
    mUnlockStaticInitLock( static_inst_lck__ ); \
    \
    return *inst; \
}


#define mDefineFactory3Param( mod, T, P0, P1, P2, funcname ) \
mGlobal(mod) ::Factory3Param<T,P0,P1,P2>& funcname()


#define mDefineFactory3ParamInClasswKW( T, P0, P1, P2, funcname, kw ) \
static ::Factory3Param<T,P0,P1,P2>& funcname(); \
virtual uiString factoryDisplayName() const \
{ return ::toUiString( factoryKeyword() ); } \
virtual const char* factoryKeyword() const { return kw; }
#define mDefineFactory3ParamInClass( T, P0, P1, P2, funcname ) \
    mDefineFactory3ParamInClasswKW( T, P0, P1, P2, funcname, nullptr )


#define mImplFactory3Param( T, P0, P1, P2,funcname ) \
::Factory3Param<T,P0,P1,P2>& funcname() \
{ \
    mLockStaticInitLock( static_inst_lck__ ); \
    static PtrMan< ::Factory3Param<T,P0,P1,P2> > \
	inst(new ::Factory3Param<T,P0,P1,P2>); \
    mUnlockStaticInitLock( static_inst_lck__ ); \
\
    return *inst; \
}

#undef mCreateImpl
#undef mAddCreator

