#ifndef enums_h
#define enums_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		4-2-1994
 Contents:	Enum <--> string conversion
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "iopar.h"
#include "uistring.h"

#include "namedobj.h"

/*!
\brief Holds data pertinent to a certain enum.
*/


mExpClass(Basic) EnumDef : public NamedObject
{
public:
			EnumDef(const char* nm,const char* s[],short nrs=0);
    bool		isValidName(const char*) const;
    int			convert(const char* s) const;
    const char*		convert(int i) const;
    uiString		getUiString(int i) const;
    const char**	names() const { return names_; }
    const uiStringSet&	strings() const;

    int			size() const;

protected:
    uiStringSet		uistrings_;
    const char**	names_;
    short		nrsign_;
};

template <class ENUM>
mClass(Basic) EnumDefImpl : public EnumDef
{ mODTextTranslationClass(EnumDef)
public:
		EnumDefImpl(const char* nm,const char* s[],short nrs=0);
    bool	parse(const char* txt,ENUM& res) const;
    bool	parse(const IOPar& par,const char* key,ENUM& res) const;
    ENUM	parse(const char* txt) const;
    const char* toString(ENUM theenum) const;
    uiString	toUiString(ENUM theenum) const;
private:
    void	initUiStrings();

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
  * DeclareEnumUtils(enm) will make sure the enum will have a string conversion.
  * DefineEnumNames(clss,enm,deflen,prettynm) defines the names.
  * For namespaces, you can use DeclareNameSpaceEnumUtils only

  The 'Declare' macros should be placed in the public section of the class.
  Example of usage:

  in myclass.h:
  \code
  #include <enums.h>

  class MyClass
  {
  public:
      enum State  { Good, Bad, Ugly };
		  DeclareEnumUtils(State)
      enum Type   { Yes, No, Maybe };
       	          DeclareEnumUtils(Type)

		  // rest of class
  };
  \endcode

  in myclass.cc:

  \code
  #include <myclass.h>
  
  DefineEnumNames(MyClass,State,1,"My class state")
	  { "Good", "Bad", "Not very handsome", 0 };
  DefineEnumNames(MyClass,Type,0,"My class type")
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

      enum 			  State { Good, Bad, Ugly };
      static const EnumDef&       StateDef();
      static const char**         StateNames();
      static bool                 parseEnum(const char*, State& );
      static bool                 parseEnum(const IOPar&,const char*key,State&);
      static int                  parseEnumState(const char*);
      static const char*          toString(State);
      static uiString		  toUiString(State);

  protected:

      static const char*          StateNames_[];
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

  const char* MyClass::Statenames_[] =
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

  const char* MyClass::Typenames_[] =
          { "Yes", "No", "Not sure", 0 };

  \endcode

  Localization is separated from the selection. Hence, if you wish to add
  translated enum-strings, you can implement your own initUiStrings() function:

 \code
 template <>
 void EnumDefImpl<MyClass::Type>::initUiStrings()
 {
     uistrings_ += tr("Yes");
     uistrings_ += tr("No");
     uistrings_ += tr("Not sure");
 }
 \endcode

 Note that the selection will still be done on the non-translated string.

-*/

#define DeclareEnumUtils(enm) \
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
    static const EnumDefImpl<enm> enm##Definition_; \
    static const char* enm##Names_[]; \
public:

#define DeclareNameSpaceEnumUtils(mod,enm) \
    mExtern(mod) const EnumDefImpl<enm>& enm##Def(); \
    mExtern(mod) const char** enm##Names();\
    extern const EnumDefImpl<enm> enm##Definition_; \
    extern const char* enm##Names_[]; \
    mExtern(mod) bool parseEnum(const IOPar&,const char*,enm&); \
    mExtern(mod) bool parseEnum(const char*,enm&); \
    mExtern(mod) bool parseEnum##enm(const char*,enm&); /*legacy */  \
    mExtern(mod) enm parseEnum##enm(const char*); \
    mExtern(mod) const char* toString(enm); \
    mExtern(mod) uiString toUiString(enm); \
    mExtern(mod) const char* get##enm##String(enm); /*legacy */

#define _DefineEnumNames(prefix,enm,deflen,prettynm) \
const EnumDefImpl<prefix::enm> prefix::enm##Definition_ ( prettynm, \
				prefix::enm##Names_, deflen ); \
const EnumDefImpl<prefix::enm>& prefix::enm##Def() \
{ return prefix::enm##Definition_; } \
const char** prefix::enm##Names() \
{ return prefix::enm##Def().names(); } \
bool prefix::parseEnum##enm(const char* txt, prefix::enm& res ) \
{ \
    const bool isok = prefix::parseEnum( txt, res ); \
    if ( !isok ) res = (prefix::enm) 0; \
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
{ return enm##Def().toString( theenum ); } \
uiString prefix::toUiString( prefix::enm theenum ) \
{ return enm##Def().toUiString( theenum ); } \
const char* prefix::enm##Names_[] =


#define DefineEnumNames(clss,enm,deflen,prettynm) \
_DefineEnumNames( clss, enm, deflen, prettynm )

#define DefineNameSpaceEnumNames(nmspc,enm,deflen,prettynm) \
_DefineEnumNames( nmspc, enm, deflen, prettynm )

template <class ENUM> inline
EnumDefImpl<ENUM>::EnumDefImpl( const char* nm, const char* nms[], short nrs )
    : EnumDef( nm, nms, nrs )
{
    initUiStrings();
    if ( uistrings_.size()!=size() )
    {
	pErrMsg( "Wrong number of uistrings" );
    }

    for ( int idx=uistrings_.size(); idx<size(); idx++ )
	uistrings_ += ::toUiString(names_[idx]);
}

template <class ENUM> inline
bool EnumDefImpl<ENUM>::parse(const char* txt, ENUM& res ) const
{
    const int idx = isValidName( txt )
	?  convert( txt )
	: -1;
    if ( idx<0 )
	return false;

    res = (ENUM) idx;
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
ENUM EnumDefImpl<ENUM>::parse(const char* txt) const
{
    return (ENUM) convert( txt );
}


template <class ENUM> inline
const char* EnumDefImpl<ENUM>::toString( ENUM theenum ) const
{
    return convert( (int) theenum );
}


template <class ENUM> inline
uiString EnumDefImpl<ENUM>::toUiString( ENUM theenum ) const
{ return getUiString((int) theenum ); }


template <class ENUM> inline
void EnumDefImpl<ENUM>::initUiStrings()
{
    for ( int idx=0; names_[idx]; idx++ )
	uistrings_ += ::toUiString(names_[idx]);
}



#endif

