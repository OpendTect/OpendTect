#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 1994, Aug 2006
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "ptrman.h"
#include "typeset.h"
#include "uistring.h"
#include "notify.h"


/*!\brief Base class for generalized static factories that can deliver
  instances of a certain class. Subclasses Factory0Param, Factory1Param,
  Factory2Param and Factory3Param deliver objects of a type T.

  The number of Parameters corresponds with the number of arguments that need
  to be passed to the constructor of subclass objects.

  Each subclass of the base class T must add itself to the factory when
  the application starts up (or the plugin loads), usually in a function called
  initClass().

  \code

  // mybase.h
  class MyBase
  {
  public:

			mDefineFactory1ParamInClass( MyClass, aParamType,
						     factory );
			// will add FactoryType& factory() to interface

      // For example:
      virtual int	aVirtualFunc()	    = 0;

  };

  // myclass.h
  class MyClass : public MyBase
  {
  public:

		MyClass(aParamType);

		mDefaultFactoryInstantiation1Param( MyBase, MyClass,
				    aParamType, "MyClass", tr("My Class") );

      virtual int aVirtualFunc();
  };

  // mybase.cc
  mImplClassFactory( MyClass, factory );

  \endcode

  Thusm you need to use two macros in the base class:
  * in the .h file mDefineFactoryxParamInClass
  * in the .cc file mImplClassFactory

  In the subclass you usually neeed only one macro:
  * in the .h mDefaultFactoryInstantiationxParam

  The static 'factory() now present in the base class can produce not only
  the requested subclass objects, but also lists of available keys and user
  display names, and other handy tools. Most of these functions come from this
  FactoryBase class.

  You can get a notification when any object was created using the
  objectCreated notifier. Note that you'll get it before the owner has a chance
  to do anything with it.

*/

mExpClass(Basic) FactoryBase : public CallBacker
{
public:

			 FactoryBase()
			     : lastcreatedidx_(-1)
			     , defaultkeyidx_(0) {}
    virtual		~FactoryBase()		{ detachAllNotifiers(); }

    int			size() const		{ return keys_.size(); }
    bool		isEmpty() const		{ return keys_.isEmpty();}

			// Following will search keys and aliases
    int			indexOf(const char* ky) const;
    inline bool		isPresent( const char* ky ) const
						{ return indexOf(ky) >= 0; }

    const char*		key(int) const;
    uiString		userName(int) const;

    const BufferStringSet& getKeys() const	{ return keys_; }
    const uiStringSet&	getUserNames() const	{ return usernames_; }

    const char*		defaultKey() const;
    void		setDefaultIdx(int);
    void		setDefaultKey(const char*);

    const char*		keyOfLastCreated() const;

protected:

    static bool		getKeyAndAliases(const char*,
						 BufferString&,BufferString&);
    void		addNames(const char*,const uiString&);
    void		setNames(int,const char*,const uiString&);

    BufferStringSet	keys_;
    BufferStringSet	aliases_;
    uiStringSet		usernames_;
    Threads::Atomic<int> defaultkeyidx_;
    mutable Threads::Atomic<int> lastcreatedidx_;

};


/*!\brief Factory for objects with no parameters in constructor.
	  See FactoryBase for details. */

template <class T>
mClass(Basic) Factory0Param : public FactoryBase
{ mODTextTranslationClass(Factory);
public:

    typedef			T* (*Creator)();
    typedef T			ObjType;
    typedef Factory0Param<T>	ThisType;

			Factory0Param()
			    : objectCreated(this)	{}

    inline int		addCreator(Creator,const char* ky,
				   uiString unm=uiString::empty());
				/*!< If ky is already present, old creator is
				     replaced. ky can be a FileMultiString
				     allowing multiple keys */

    inline T*		create(const char* ky) const;
    inline T*		createAny() const;
    inline T*		createSuitable() const	    { return createAny(); }

    CNotifier< ThisType, T* >	objectCreated;

protected:

    TypeSet<Creator>	creators_;

};


/*!\brief Factory for objects with 1 parameter in constructor.
	  See FactoryBase for details. */

template <class T, class P>
mClass(Basic) Factory1Param : public FactoryBase
{ mODTextTranslationClass(Factory1Param);
public:

    typedef			T* (*Creator)(P);
    typedef T			ObjType;
    typedef Factory1Param<T,P>	ThisType;

			Factory1Param()
			    : objectCreated(this)	{}

    inline int		addCreator(Creator,const char* ky,
				   uiString usernm =uiString::empty());

    inline T*		create(const char*,P) const;
    inline T*		createSuitable(P) const;

    CNotifier< ThisType, T* >	objectCreated;

protected:

    TypeSet<Creator>	creators_;

};


/*!\brief Factory using 2 parameters for construction of new objects. */

template <class T, class P0, class P1>
mClass(Basic) Factory2Param : public FactoryBase
{
public:

    typedef				T* (*Creator)(P0,P1);
    typedef T				ObjType;
    typedef Factory2Param<T,P0,P1>	ThisType;

			Factory2Param()
			    : objectCreated(this)	{}

    inline int		addCreator(Creator,const char* ky,
				   uiString usernm=uiString::empty());

    inline T*		create(const char*,P0,P1) const;
    inline T*		createSuitable(P0,P1) const;

    CNotifier< ThisType, T* >	objectCreated;

protected:

    TypeSet<Creator>	creators_;

};


/*!\brief Factory using 3 parameters for construction of new objects. */

template <class T, class P0, class P1, class P2>
mClass(Basic) Factory3Param : public FactoryBase
{
public:

    typedef				T* (*Creator)(P0,P1,P2);
    typedef T				ObjType;
    typedef Factory3Param<T,P0,P1,P2>	ThisType;

			Factory3Param()
			    : objectCreated(this)	{}

    inline T*		create(const char*,P0,P1,P2) const;
    inline T*		createSuitable(P0,P1,P2) const;

    inline int		addCreator(Creator,const char* ky,
				   uiString usernm=uiString::empty());

    CNotifier< ThisType, T* >	objectCreated;

protected:

    TypeSet<Creator>	creators_;

};


#define mCreateImpl( toret ) \
    const int idx = indexOf( ky ); \
    if ( idx < 0 ) \
	return 0; \
    lastcreatedidx_ = idx; \
    T* ret = toret; \
    if ( ret ) \
    { \
	mSelf().objectCreated.trigger( ret ); \
    } \
    return ret;

#define mCreateSuitableImpl( toret ) \
    for ( int idx=0; idx<keys_.size(); idx++ ) \
    { \
	T* newinst = toret; \
	if ( newinst ) \
	    return newinst; \
    } \
    return 0;

#define mAddCreator \
\
    int idx = indexOf( ky );\
\
    if ( idx < 0 )\
    { \
	addNames( ky, unm ); \
	creators_ += cr; \
	idx = creators_.size() - 1; \
    } \
    else\
    {\
	setNames( idx, ky, unm ); \
	creators_[idx] = cr; \
    }\
\
    return idx

template <class T> inline
int Factory0Param<T>::addCreator( Creator cr, const char* ky, uiString unm )
{ mAddCreator; }
template <class T> inline
T* Factory0Param<T>::create( const char* ky ) const
{ mCreateImpl( creators_[idx]() ) }
template <class T> inline
T* Factory0Param<T>::createAny() const
{ mCreateSuitableImpl( create( keys_.get(idx) ) ) }

template <class T, class P> inline
int Factory1Param<T,P>::addCreator( Creator cr, const char* ky, uiString unm )
{ mAddCreator; }
template <class T, class P> inline
T* Factory1Param<T,P>::create( const char* ky, P p ) const
{ mCreateImpl( creators_[idx]( p ) ); }
template <class T, class P> inline
T* Factory1Param<T,P>::createSuitable( P p ) const
{ mCreateSuitableImpl( create( keys_.get(idx), p ) ) }

template <class T, class P0, class P1> inline
int Factory2Param<T,P0,P1>::addCreator( Creator cr, const char* ky,
					uiString unm )
{ mAddCreator; }
template <class T, class P0, class P1> inline
T* Factory2Param<T,P0,P1>::create( const char* ky, P0 p0, P1 p1 ) const
{ mCreateImpl( creators_[idx]( p0, p1 ) ); }
template <class T, class P0, class P1> inline
T* Factory2Param<T,P0,P1>::createSuitable( P0 p0, P1 p1 ) const
{ mCreateSuitableImpl( create( keys_.get(idx), p0, p1 ) ) }

template <class T, class P0, class P1, class P2> inline
int Factory3Param<T,P0,P1,P2>::addCreator( Creator cr, const char* ky,
					   uiString unm )
{ mAddCreator; }
template <class T, class P0, class P1, class P2> inline
T* Factory3Param<T,P0,P1,P2>::create( const char* ky, P0 p0, P1 p1, P2 p2) const
{ mCreateImpl( creators_[idx]( p0, p1, p2 ) ); }
template <class T, class P0, class P1, class P2> inline
T* Factory3Param<T,P0,P1,P2>::createSuitable( P0 p0, P1 p1, P2 p2 ) const
{ mCreateSuitableImpl( create( keys_.get(idx), p0, p1, p2 ) ) }


#define mDefFactoryClassFns( kw, funcname ) \
    static FactoryType&		funcname(); \
    virtual uiString		factoryDisplayName() const \
				{ return ::toUiString( factoryKeyword() ); } \
    virtual const char*		factoryKeyword() const { return kw; }


// instantiation in .cc file:

#define mImplGenFactory( facttyp, fullfuncname ) \
\
facttyp& fullfuncname() \
{ \
    mDefineStaticLocalObject( PtrMan<facttyp>, inst, (new facttyp) ); \
    return *inst; \
}

// for 'global' factories, in .cc
#define mImplFactory( T, funcname ) \
    mImplGenFactory( T##FactoryType, funcname )

// for class factories, in .cc
#define mImplClassFactory( T, funcname ) \
    mImplGenFactory( T::FactoryType, T::funcname )


//---- for objects with constructors that take no arguments

// for 'global' factories, in .h
#define mDefineFactory0Param( mod, T, funcname ) \
    typedef ::Factory0Param<T>		T##FactoryType; \
    mGlobal(mod) T##FactoryType&	funcname()

// in-class factories, if your key is provided by another source, in .h
#define mDefineFactory0ParamInClasswKW( T, funcname, kw ) \
    typedef Factory0Param<T>		FactoryType; \
					mDefFactoryClassFns(kw,funcname)

// for usual in-class factories, in .h
#define mDefineFactory0ParamInClass( T, funcname ) \
    mDefineFactory0ParamInClasswKW( T, funcname, 0 )

// shorthand
#define mDefineFactory( mod, T, funcname ) \
    mDefineFactory0Param( mod, T, funcname )

// shorthand
#define mDefineFactoryInClass( T, funcname ) \
    mDefineFactory0ParamInClass( T, funcname )

// shorthand
#define mDefineFactoryInClasswKW( T, funcname, kw ) \
    mDefineFactory0ParamInClasswKW( T, funcname, kw )


//---- objects with constructors that take a single argument

// for 'global' factories, in .h
#define mDefineFactory1Param( mod, T, P, funcname ) \
    typedef ::Factory1Param<T,P>	T##FactoryType; \
    mGlobal(mod) T##FactoryType&	funcname()

// in-class factories, if your key is provided by another source, in .h
#define mDefineFactory1ParamInClasswKW( T, P, funcname, kw ) \
    typedef Factory1Param<T,P>		FactoryType; \
					mDefFactoryClassFns(kw,funcname)

// for usual in-class factories, in .h
#define mDefineFactory1ParamInClass( T, P, funcname ) \
    mDefineFactory1ParamInClasswKW( T, P, funcname, 0 )


//---- objects with constructors that take a two arguments

// for 'global' factories, in .h
#define mDefineFactory2Param( mod, T, P1, P2, funcname ) \
    typedef ::Factory2Param<T,P1,P2>	T##FactoryType; \
    mGlobal(mod) T##FactoryType&	funcname()

// in-class factories, if your key is provided by another source, in .h
#define mDefineFactory2ParamInClasswKW( T, P1, P2, funcname, kw ) \
    typedef Factory2Param<T,P1,P2>	FactoryType; \
					mDefFactoryClassFns(kw,funcname)

// for usual in-class factories, in .h
#define mDefineFactory2ParamInClass( T, P1, P2, funcname ) \
    mDefineFactory2ParamInClasswKW( T, P1, P2, funcname, 0 )

//---- objects with constructors that take a three arguments

// for 'global' factories, in .h
#define mDefineFactory3Param( mod, T, P1, P2, P3, funcname ) \
    typedef ::Factory3Param<T,P1,P2,P3>	T##FactoryType; \
    mGlobal(mod) T##FactoryType&	funcname()

// in-class factories, if your key is provided by another source, in .h
#define mDefineFactory3ParamInClasswKW( T, P1, P2, P3, funcname, kw ) \
    typedef Factory3Param<T,P1,P2,P3>	FactoryType; \
					mDefFactoryClassFns(kw,funcname)

// for usual in-class factories, in .h
#define mDefineFactory3ParamInClass( T, P1, P2, P3, funcname ) \
    mDefineFactory3ParamInClasswKW( T, P1, P2, P3, funcname, 0 )


#undef mCreateImpl
#undef mAddCreator


// Macros to implement required functions in a standard way, in .cc file

#define mDefaultFactoryStringImpl \
    const char*		factoryKeyword() const { return sFactoryKeyword(); } \
    uiString		factoryDisplayName() const \
					{ return sFactoryDisplayName(); }

#define mDefaultStaticFactoryStringDeclaration \
    static const char*	sFactoryKeyword(); \
    static uiString	sFactoryDisplayName()

#define mDefaultFactoryInitClassImpl( baseclss, createfunc ) \
{ \
    baseclss::factory().addCreator((baseclss::FactoryType::Creator)createfunc, \
		    sFactoryKeyword(), sFactoryDisplayName()); \
}

#define mDefaultFactoryInstantiationBase( keywrd, usernm ) \
    mDefaultFactoryStringImpl \
    static const char*	sFactoryKeyword() { return keywrd; } \
    static uiString	sFactoryDisplayName() { return usernm; } \
    static void		initClass()


#define mDefaultFactoryCreatorImpl0Param( baseclss, clss ) \
static mDeprecated clss* createInstance() { return new clss; } \
static baseclss*	create##baseclss##Instance() { return new clss; } \

#define mDefaultFactoryInstantiation0Param( baseclss, clss, keywrd, usernm ) \
    mDefaultFactoryCreatorImpl0Param( baseclss, clss ); \
    mDefaultFactoryInstantiationBase( keywrd, usernm ) \
    mDefaultFactoryInitClassImpl( baseclss, create##baseclss##Instance )

#define mDefaultFactoryInstantiation( baseclss, clss, keywrd, usernm ) \
    mDefaultFactoryInstantiation0Param( baseclss, clss, keywrd, usernm )


#define mDefaultFactoryCreatorImpl1Param( baseclss, clss, parclss ) \
static mDeprecated clss* createInstance( parclss p1 ) \
			{ return new clss( p1 ); } \
static baseclss*	create##baseclss##Instance( parclss p1 ) \
			{ return new clss( p1 ); }


#define mDefaultFactoryInstantiation1Param( baseclss, clss, parclss, \
					    keywrd, usernm ) \
    mDefaultFactoryCreatorImpl1Param( baseclss, clss, parclss ) \
    mDefaultFactoryInstantiationBase( keywrd, usernm ) \
    mDefaultFactoryInitClassImpl( baseclss, create##baseclss##Instance )


#define mDefaultFactoryCreatorImpl2Param( baseclss, clss, parclss1, parclss2 ) \
static mDeprecated clss* createInstance( parclss1 p1, parclss2 p2 ) \
			{ return new clss( p1, p2 ); } \
static baseclss*	create##baseclss##Instance( parclss1 p1, parclss2 p2 ) \
			{ return new clss( p1, p2 ); }

#define mDefaultFactoryInstantiation2Param( baseclss, clss, parclss1, \
					    parclss2, keywrd, usernm ) \
    mDefaultFactoryCreatorImpl2Param( baseclss, clss, parclss1, parclss2 ) \
    mDefaultFactoryInstantiationBase( keywrd, usernm ) \
    mDefaultFactoryInitClassImpl( baseclss, create##baseclss##Instance )


#define mDefaultFactoryCreatorImpl3Param( baseclss, clss, parclss1, parclss2, \
								    parclss3 ) \
static mDeprecated clss* createInstance( parclss1 p1, parclss2 p2, \
								parclss3 p3 ) \
			{ return new clss( p1, p2, p3 ); } \
static baseclss*	create##baseclss##Instance( parclss1 p1, parclss2 p2, \
								 parclss3 p3) \
			{ return new clss( p1, p2, p3 ); }

#define mDefaultFactoryInstantiation3Param( baseclss, clss, parclss1,\
					    parclss2, parclss3, keywrd, \
								     usernm ) \
    mDefaultFactoryCreatorImpl3Param( baseclss, clss, parclss1, parclss2, \
								   parclss3 ) \
    mDefaultFactoryInstantiationBase( keywrd, usernm ) \
    mDefaultFactoryInitClassImpl( baseclss, create##baseclss##Instance )
