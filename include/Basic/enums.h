#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		4-2-1994
 Contents:	Enum <--> string conversion
________________________________________________________________________

-*/

#include "basicmod.h"
#include "iopar.h"
#include "uistring.h"
#include "bufstringset.h"

#include "namedobj.h"

/*!
\brief Holds data pertinent to a certain enum. It does not know the enum
values themselves, but treat them as integers.

In most cases, the inherited EnumDefImpl<ENUM> is used, but a stand-alone
EnumDef can be used if one wants to use combinations of enums that
are not available as EnumDefImpl.

A stand-alone EnumDef can be created as follows to create a combo box
with all enums in Stats::Type without the average.

\code
EnumDef def = Stats::TypeDef();
def.remove( def.getKey(Stats::Average) );

new uiComboBox( this, def );
\endcode


*/


mExpClass(Basic) EnumDef : public NamedObject
{
public:
				EnumDef();
				EnumDef(const char* nm,const char* s[],
					short nrs=0);
    bool			isValidKey(const char*) const;
    int				indexOf(const char* s) const;
    int				indexOf(int enumval) const;
    int				getEnumValForIndex(int) const;
    const char*			getKeyForIndex(int i) const;
    uiString			getUiStringForIndex(int i) const;
    void			setUiStringForIndex(int,const uiString&);
    const BufferStringSet&	keys() const { return keys_; }
    const uiStringSet&		strings() const { return uistrings_; }

    const char*			getIconFileForIndex(int idx) const;
    void			setIconFileForIndex(int idx,const char*);

    int				size() const;

    //Expert use only!
    void			remove(const char* key);
    void			add(const char* key, const uiString&,
				    int enumval, const char* iconfile);

protected:
    void		fillUiStrings();
    uiStringSet		uistrings_;
    BufferStringSet	keys_;
    TypeSet<int>	enums_;
    BufferStringSet	iconfiles_;
public:
    //Legacy
    bool		isValidName(const char* key) const
			{ return isValidKey(key); }
    const char*		convert(int idx) const { return getKeyForIndex(idx); }
    int			convert(const char* txt) const { return indexOf(txt); }
};

template <class ENUM>
mClass(Basic) EnumDefImpl : public EnumDef
{ mODTextTranslationClass(EnumDefImpl);
public:
			EnumDefImpl(const char* nm,const char* s[],short nrs=0);
    bool		parse(const char* txt,ENUM& res) const;
    bool		parse(const IOPar& par,const char* key,ENUM& res) const;
    ENUM		parse(const char* txt) const;
    ENUM		getEnumForIndex(int idx) const;
    const char*		getKey(ENUM theenum) const;
    uiString		toUiString(ENUM theenum) const;
private:
    void		init();
public:
    //Legacy
    const char*		toString(ENUM theenum) const { return getKey(theenum); }

};


/*!
\ingroup Basic
\brief Some utilities surrounding the often needed enum <-> string table.

  The function EnumDef::convert returns the enum (integer) value from a text
  string. The first arg is string you wish to convert to the enum, the second
  is the array with enum names. Then, the integer value of the first enum value
  (also returned when no match is found) and the number of characters to be
  matched (0=all). Make absolutely sure the char** definition has a closing
  ' ... ,0 };'.

  Normally, you'll have a class with an enum member. In that case, you'll want
  to use the EnumDef classes. These are normally almost hidden by a few
  simple macros:
  * mDeclareEnumUtils(enm) will make sure the enum will have a string conversion
  * mDefineEnumUtils(clss,enm,prettynm) defines the names.
  * For namespaces, you can use mDeclareNameSpaceEnumUtils only

  The 'Declare' macros should be placed in the public section of the class.
  Example of usage:

  in myclass.h:
  \code
  #include <enums.h>

  class MyClass
  {
  public:
      enum State  { Good, Bad, Ugly };
		  mDeclareEnumUtils(State)
      enum Type   { Yes, No, Maybe };
	          mDeclareEnumUtils(Type)

		  // rest of class
  };
  \endcode

  in myclass.cc:

  \code
  #include <myclass.h>

  mDefineEnumUtils(MyClass,State,"My class state")
	  { "Good", "Bad", "Not very handsome", 0 };
  mDefineEnumUtils(MyClass,Type,"My class type")
          { "Yes", "No", "Not sure", 0 };
  \endcode

  Note the '1' in the first one telling the EnumDef that only one character
  needs	to be matched when converting string -> enum. The '0' in the second
  indicates that the entire string must match.

  This will expand to (added newlines, removed some superfluous stuff:

  \code

  class MyClass
  {
  public:

      enum			  State { Good, Bad, Ugly };
      static const EnumDef&       StateDef();
      static const char**         StateNames();
      static bool                 parseEnum(const char*, State& );
      static bool                 parseEnum(const IOPar&,const char*key,State&);
      static int                  parseEnumState(const char*);
      static const char*          toString(State);
      static uiString		  toUiString(State);

  protected:

      static const char*	  StateKeys_[];
      static const EnumDef        StateDefinition_;

  // similar for Type

  };

  \endcode

  and, in myclass.cc:

  \code

  const EnumDef& MyClass::StateDef()    { return StateDefinition_; }

  const EnumDef MyClass::StateDefinition_("My class state",
						MyClass::Statenames,1);

  bool MyClass::parseEnum(const char* txt, State& res ) \
  { \
      const int idx = StateDef().isValidName( txt ) \
          ?  StateDef().convert( txt ) \
          : -1; \
      if ( idx<0 ) \
          return false; \
    \
      res = (State) idx; \
      return true; \
  } \
  bool MyClass::parseEnum( const IOPar& par, const char* key, State& res ) \
  { return parseEnum( par.find( key ), res ); } \
  MyClass::State MyClass::parseEnumState(const char* txt) \
  { \
      return (MyClass::State) StateDef().convert( txt ); \
  } \

  const char* MyClass::StateKeys_[] =
          { "Good", "Bad", "Not very handsome", 0 };


  const EnumDef& MyClass::TypeDef()   { return TypeDefinition_; }
  const EnumDef MyClass::TypeDefinition_("My class type",MyClass::Typenames, 0);
  bool MyClass::parseEnum(const char* txt, Type& res ) \
  { \
      const int idx = TypeDef().isValidName( txt ) \
          ?  TypeDef().convert( txt ) \
          : -1; \
      if ( idx<0 ) \
          return false; \
    \
      res = (Type) idx; \
      return true; \
  } \
  bool MyClass::parseEnum( const IOPar& par, const char* key, Type& res ) \
  { return parseEnum( par.find( key ), res ); } \
  MyClass::Type MyClass::parseEnumType(const char* txt) \
  { \
      return (MyClass::Type) TypeDef().convert( txt ); \
  } \

  const char* MyClass::TypeKeys_[] =
          { "Yes", "No", "Not sure", 0 };

  \endcode

  Localization is separated from the selection. Hence, if you wish to add
  translated enum-strings, you can implement your own init() function:

 \code
 template <>
 void EnumDefImpl<MyClass::Type>::init()
 {
     uistrings_ += tr("Yes");
     uistrings_ += tr("No");
     uistrings_ += tr("Not sure");
 }
 \endcode

 Note that the selection will still be done on the non-translated string.

-*/

#define mDeclareEnumUtils(enm) \
public: \
    static const EnumDefImpl<enm>& enm##Def(); \
    static const char** enm##Names();\
    static bool parseEnum##enm(const char*,enm&);  /*legacy */ \
    static bool parseEnum(const char*,enm&); \
    static bool parseEnum(const IOPar&,const char*,enm&); \
    static enm parseEnum##enm(const char*);  \
    static const char* toString(enm); \
    static uiString toUiString(enm); \
    static const char* get##enm##String(enm); /*legacy */ \
protected: \
    static const char* enm##Keys_[]; \
    static ConstPtrMan<EnumDefImpl<enm> > enm##Definition_; \
public:

#define mDeclareNameSpaceEnumUtils(mod,enm) \
    mExtern(mod) const EnumDefImpl<enm>& enm##Def(); \
    mExtern(mod) const char** enm##Names();\
    extern const char* enm##Keys_[]; \
    mExtern(mod) bool parseEnum(const IOPar&,const char*,enm&); \
    mExtern(mod) bool parseEnum(const char*,enm&); \
    mExtern(mod) bool parseEnum##enm(const char*,enm&); /*legacy */  \
    mExtern(mod) enm parseEnum##enm(const char*); \
    mExtern(mod) const char* toString(enm); \
    mExtern(mod) uiString toUiString(enm); \
    mExtern(mod) const char* get##enm##String(enm); /*legacy */

#define _DefineEnumNames(prefix,enm,deflen,prettynm) \
const EnumDefImpl<prefix::enm>& prefix::enm##Def() \
{ \
    if ( !enm##Definition_ ) \
    { \
	EnumDefImpl<prefix::enm>* newdef = \
	    new EnumDefImpl<prefix::enm>(prettynm,prefix::enm##Keys_,deflen); \
	enm##Definition_.setIfNull( newdef,true ); \
    } \
 \
    return *enm##Definition_; \
} \
const char** prefix::enm##Names() \
{ return prefix::enm##Keys_; } \
bool prefix::parseEnum##enm(const char* txt, prefix::enm& res ) \
{ \
    const bool isok = prefix::parseEnum( txt, res ); \
    if ( !isok ) res = sCast(prefix::enm,0); \
    return isok; \
} \
bool prefix::parseEnum(const char* txt, prefix::enm& res ) \
{ return prefix::enm##Def().parse( txt, res ); } \
bool prefix::parseEnum( const IOPar& par, const char* key, prefix::enm& res ) \
{ return prefix::enm##Def().parse( par, key, res ); } \
prefix::enm prefix::parseEnum##enm(const char* txt) \
{ return prefix::enm##Def().parse( txt ); } \
const char* prefix::get##enm##String( prefix::enm theenum ) \
{ return prefix::toString( theenum ); } \
const char* prefix::toString( prefix::enm theenum ) \
{ return enm##Def().getKey( theenum ); } \
uiString prefix::toUiString( prefix::enm theenum ) \
{ return enm##Def().toUiString( theenum ); } \
const char* prefix::enm##Keys_[] =


#define DefineEnumNames(clss,enm,deflen,prettynm) \
ConstPtrMan<EnumDefImpl<clss::enm> > clss::enm##Definition_ = nullptr; \
_DefineEnumNames( clss, enm, deflen, prettynm )

#define DefineNameSpaceEnumNames(nmspc,enm,deflen,prettynm) \
static ConstPtrMan<EnumDefImpl<nmspc::enm> > enm##Definition_ = nullptr; \
_DefineEnumNames( nmspc, enm, deflen, prettynm )

//New Defs
#define mDefineEnumUtils(clss,enm,prettynm) \
DefineEnumNames(clss,enm,0,prettynm)

#define mDefineNameSpaceEnumUtils(nmspc,enm,prettynm) \
DefineNameSpaceEnumNames(nmspc,enm,0,prettynm)

//Legacy
#define DeclareEnumUtils(enm) mDeclareEnumUtils(enm)
#define DefineEnumUtils(clss,enm,prettynm) mDefineEnumUtils(clss,enm,prettynm)
#define DefineNameSpaceEnumUtils(nmspc,enm,prettynm) \
mDefineNameSpaceEnumUtils(nmspc,enm,prettynm)
#define DeclareNameSpaceEnumUtils(mod,enm) mDeclareNameSpaceEnumUtils(mod,enm)

#define mEnumTr(str) tr(str)


template <class ENUM> inline
EnumDefImpl<ENUM>::EnumDefImpl( const char* nm, const char* nms[], short nrs )
    : EnumDef( nm, nms, nrs )
{
    init();
    if ( !uistrings_.isEmpty() && uistrings_.size()!=size() )
    {
	pErrMsg( "Wrong number of uistrings" );
    }

    for ( int idx=uistrings_.size(); idx<size(); idx++ )
	uistrings_ += ::toUiString( keys_.get(idx) );

    if ( !iconfiles_.isEmpty() && iconfiles_.size()!=size() )
    {
	pErrMsg( "Wrong number of iconfiles" );
    }
}

template <class ENUM> inline
bool EnumDefImpl<ENUM>::parse(const char* txt, ENUM& res ) const
{
    const int idx = isValidKey( txt )
	?  indexOf( txt )
	: -1;
    if ( idx<0 )
	return false;

    res = (ENUM) enums_[idx];
    return true;
}


template <class ENUM> inline
bool EnumDefImpl<ENUM>::parse( const IOPar& par, const char* key,
			       ENUM& res ) const
{
    const char* val = par.find( key );
    return parse( val, res );
}


template <class ENUM> inline
ENUM EnumDefImpl<ENUM>::getEnumForIndex( int idx ) const
{
    return (ENUM) enums_[idx];
}


template <class ENUM> inline
ENUM EnumDefImpl<ENUM>::parse(const char* txt) const
{
    return getEnumForIndex( indexOf( txt ) );
}


template <class ENUM> inline
const char* EnumDefImpl<ENUM>::getKey( ENUM theenum ) const
{
    if ( !size() )
	return 0;

    const int idx = enums_.indexOf( (int) theenum );
    return keys_.get(idx>=0?idx:0).buf();
}


template <class ENUM> inline
uiString EnumDefImpl<ENUM>::toUiString( ENUM theenum ) const
{
    const int idx = enums_.indexOf( (int) theenum );
    return getUiStringForIndex( idx );
}


template <class ENUM> inline
void EnumDefImpl<ENUM>::init()
{
     fillUiStrings();
}


