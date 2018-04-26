#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
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
Note: EnumDef is assumed to remain in memory.

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
				EnumDef(const char* nm,const char* s[]);

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
    void			fillUiStringsWithKeys();

protected:

    uiStringSet		uistrings_;
    BufferStringSet	keys_;
    TypeSet<int>	enums_;
    BufferStringSet	iconfiles_;

};


template <class ENUM>
mClass(Basic) EnumDefImpl : public EnumDef
{ mODTextTranslationClass(EnumDefImpl);
public:

			EnumDefImpl(const char* nm,const char* s[]);

    bool		parse(const char* txt,ENUM& res) const;
    bool		parse(const IOPar& par,const char* key,ENUM& res) const;
    ENUM		parse(const char* txt) const;
    ENUM		getEnumForIndex(int idx) const;
    const char*		getKey(ENUM theenum) const;
    uiString		toUiString(ENUM theenum) const;

private:

    void		init();

};


/*!
\ingroup Basic
\brief Some utilities surrounding the often needed enum <-> string table.

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

  This will expand to (added newlines, removed some superfluous stuff:

  \code

  class MyClass
  {
  public:

      enum				State { Good, Bad, Ugly };
      static const EnumDefImpl<State>& StateDef();

  private:
      static const PtrMan<EnumDefImpl<State> >	StateDefinition_;

  // similar for Type

  };

  \endcode

  and, in myclass.cc:

  \code
  const PtrMan<EnumDefImpl<State> > MyClass::StateDefinition_ = 0;
  const EnumDef& MyClass::StateDef()
  { return *StateDefinition_.createIfNull(); }
  \endcode

  Localization is separated from the selection. Hence, if you wish to add
  translated enum-strings, you can implement your own init() function:

 \code
 template <>
 void EnumDefImpl<MyClass::Type>::init()
 {
     uistrings_ += uiStrings::sYes();
     uistrings_ += uiStrings::sNo();
     uistrings_ += mEnumTr("Not sure","disambiguation");
 }
 \endcode

 Note that the selection will still be done on the non-translated string.

-*/

#define mDeclareEnumUtils(enm) \
public: \
    static const EnumDefImpl<enm>& enm##Def(); \
    static mDeprecated const char** enm##Names();\
    static mDeprecated bool parseEnum##enm(const char*,enm&);  /*legacy */ \
    static mDeprecated bool parseEnum(const char*,enm&); \
    static mDeprecated bool parseEnum(const IOPar&,const char*,enm&); \
    static mDeprecated enm parseEnum##enm(const char*);  \
    static const char* toString(enm); \
    static uiString toUiString(enm); \
    static mDeprecated const char* get##enm##String(enm); /*legacy */ \
private: \
    static EnumDefImpl<enm>* enm##CreateDef(); \
    static const char* enm##Keys_[]; \
    static ConstPtrMan<EnumDefImpl<enm> > enm##Definition_; \
public:

#define mDeclareNameSpaceEnumUtils(mod,enm) \
    mExtern(mod) const EnumDefImpl<enm>& enm##Def(); \
    mExtern(mod) mDeprecated const char** enm##Names();\
    mExtern(mod) mDeprecated bool parseEnum(const IOPar&,const char*,enm&); \
    mExtern(mod) mDeprecated bool parseEnum(const char*,enm&); \
    mExtern(mod) mDeprecated bool parseEnum##enm(const char*,enm&); /*legacy */\
    mExtern(mod) mDeprecated enm parseEnum##enm(const char*); \
    mExtern(mod) const char* toString(enm); \
    mExtern(mod) uiString toUiString(enm); \
    mExtern(mod) mDeprecated const char* get##enm##String(enm); /*legacy */

//Legacy
#define DeclareEnumUtils(enm) mDeclareEnumUtils(enm)
#define DeclareNameSpaceEnumUtils(mod,enm) \
mDeclareNameSpaceEnumUtils(mod,enm)

#define _mDefineEnumUtils(prefix,enm,createfunc,prettynm) \
const EnumDefImpl<prefix::enm>& prefix::enm##Def() \
{ return *enm##Definition_.createIfNull( createfunc ); } \
bool prefix::parseEnum##enm(const char* txt, prefix::enm& res ) \
{ \
    const bool isok = prefix::enm##Def().parse( txt, res ); \
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
{ return enm##Def().getKey( theenum ); } \
uiString prefix::toUiString( prefix::enm theenum ) \
{ return enm##Def().toUiString( theenum ); } \


#define mDefineEnumUtils(clss,enm,prettynm) \
EnumDefImpl<clss::enm>* clss::enm##CreateDef() \
{ return new EnumDefImpl<clss::enm>( prettynm, clss::enm##Keys_ ); } \
const char** clss::enm##Names() { return clss::enm##Keys_ ; } \
ConstPtrMan<EnumDefImpl<clss::enm> > clss::enm##Definition_ = 0; \
_mDefineEnumUtils( clss, enm, clss::enm##CreateDef, prettynm ); \
const char* clss::enm##Keys_[] =

#define mDefineNameSpaceEnumUtils(nmspc,enm,prettynm) \
extern const char* nmspc##enm##Keys_[]; \
static EnumDefImpl<nmspc::enm>* nmspc##enm##CreateDef() \
{ return new EnumDefImpl<nmspc::enm>( prettynm, nmspc##enm##Keys_ ); } \
const char** nmspc::enm##Names() { return nmspc##enm##Keys_; } \
static ConstPtrMan<EnumDefImpl<nmspc::enm> > enm##Definition_ = 0; \
_mDefineEnumUtils( nmspc, enm, nmspc##enm##CreateDef, prettynm );\
const char* nmspc##enm##Keys_[] =

//Legacy
#define DefineEnumNames(clss,enm,deflen,prettynm) \
mDefineEnumUtils(clss,enm,prettynm)
#define DefineNameSpaceEnumNames(nmspc,enm,deflen,prettynm) \
mDefineNameSpaceEnumUtils(nmspc,enm,prettynm)
#define mEnumTr(str,disambiguation) tr(str,disambiguation)

template <class ENUM> inline
EnumDefImpl<ENUM>::EnumDefImpl( const char* nm, const char* nms[] )
    : EnumDef( nm, nms )
{
    init();
    if ( uistrings_.size()!=size() )
    {
	pErrMsg( "Wrong number of uistrings" );
    }

    for ( int idx=uistrings_.size(); idx<size(); idx++ )
	uistrings_ += ::toUiString( keys_.get(idx) );

    if ( iconfiles_.size() && iconfiles_.size()!=size() )
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
