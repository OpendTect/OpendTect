#ifndef enums_H
#define enums_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		4-2-1994; 20-10-1995
 Contents:	Enum <--> string conversion and generalized reference
 RCS:		$Id: enums.h,v 1.7 2003-12-23 15:57:01 arend Exp $
________________________________________________________________________

-*/


/*!\brief Some utilities surrounding the often needed enum <-> string table.

The simple C function getEnum returns the enum (integer) value from a text
string. The first arg is string you wish to convert to the enum, the second
is the array with enum names. Then, the integer value of the first enum value
(also returned when no match is found) and the number of characters to be
matched (0=all). Make absolutely sure the char** namearr has a closing
' ... ,0 };'.

Normally, you'll have a class with an enum member. In that case, you'll want to
use the EnumDef and EnumRef classes. These are normally almost hidden by a few
simple macros:
* DeclareEnumUtils(enm) will make sure the enum will have a string conversion.
* DeclareEnumUtilsWithVar(enm,varnm) will also create an instance variable with
  accessors.
* DefineEnumNames(clss,enm,deflen,prettynm) defines the names.

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
		DeclareEnumUtilsWithVar(Type,type)

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

Note the '1' in the first one telling the EnumDef that only one character needs
to be matched when converting string -> enum. The '0' in the second indicates
that the entire string must match.

This will expand to (added newlines, removed some superfluous stuff:

\code

class MyClass
{
public:

    enum			State { Good, Bad, Ugly };
    staticEnumRef		StateRef(State&);
    static const EnumDef&	StateDef();
    static const char**		StateNamesGet();
    static const char*		StateNames[];

protected:

    static const EnumDef	StateDef_;

public:

    enum			Type { Yes, No, Maybe };
    Type			type() const { return type_; }
    void			setType(Type _e _) { type_ = _e_; }
    EnumRef			typeRef() const
				    { return EnumRef( (int&)type_, TypeDef_ ); }
    static EnumRef		TypeRef(Type&);
    static const EnumDef&	TypeDef();
    static const char**		TypeNamesGet();
    static const char*		TypeNames[];

protected:

    static const EnumDef	TypeDef_;

    Type			type_;

};

\endcode

and, in myclass.cc:

\code

EnumRef MyClass::StateRef(State& _e_) { return EnumRef((int&)_e_, StateDef_); }

const EnumDef& MyClass::StateDef()    { return StateDef_; }
const EnumDef MyClass::StateDef_      ("My class state",MyClass::Statenames,1);

const char* MyClass::Statenames[] =
        { "Good", "Bad", "Not very handsome", 0 };


EnumRef MyClass::TypeRef(Type& _e_) { return EnumRef( (int&)_e_, TypeDef_ ); }

const EnumDef& MyClass::TypeDef()   { return TypeDef_; }
const EnumDef MyClass::TypeDef_     ( "My class type", MyClass::Typenames, 0 );

const char* MyClass::Typenames[] =
        { "Yes", "No", "Not sure", 0 };

\endcode

-*/


#include <string2.h>

#ifndef __cpp__
    int getEnum(const char*,char** namearr,int startnr,int nr_chars_to_match);
    int getEnumDef(const char*,char** namearr,int startnr,int nr_chars_to_match,
	    	   int notfoundval );
#else

#include <uidobj.h>

extern "C" { int getEnum(const char*,const char**,int,int); }
extern "C" { int getEnumDef(const char*,const char**,int,int,int); }


/*\brief holds data pertinent for a certain enum */

class EnumDef : public UserIDObject
{
    friend class	EnumRef;

public:
			EnumDef( const char* nm, const char* s[], int nrs=0 )
				: UserIDObject(nm)
				, names(s)
				, nrsign(nrs)	{}

    int			convert(const char* s) const
			{ return getEnum(s,names,0,nrsign); }
    const char*		convert( int i ) const
			{ return names[i]; }

    int			size() const
			{ int i=0; while ( names[i] ) i++; return i; }

    const char**	names;

private:
    int		nrsign;
};


/*\brief provides a generalised reference to a certain enum */

class EnumRef
{
public:
			EnumRef( int& i, const EnumDef& ed ) : val(i) , def_(ed)
						{}
			EnumRef(const EnumRef& r) : val(r.val) , def_(r.def_)
						{}

    EnumRef&	operator=( int i )
		{ if ( i < def_.size() ) val = i; return *this; }
    EnumRef&	operator=(const char*);

    int		get() const			{ return val; }
    		operator const char*() const
						{ return def_.convert(val); }

    const EnumDef&	def() const		{ return def_; }

private:
    int&		val;
    const EnumDef&	def_;
};


#define DeclareEnumUtils(enm) \
public: \
    static EnumRef enm##Ref(enm&); \
    static const EnumDef& enm##Def(); \
    static const char* enm##Names[];\
    static const char** enm##NamesGet();\
protected: \
    static const EnumDef enm##Def_; \
public:

#define DeclareEnumUtilsWithVar(enm,varnm) \
public: \
    enm varnm() const { return varnm##_; } \
    void set##enm(enm _e_) { varnm##_ = _e_; } \
    EnumRef varnm##Ref() const \
    { return EnumRef( (int&)varnm##_, enm##Def_ ); } \
    DeclareEnumUtils(enm) \
protected: \
    enm varnm##_; \
public:

#define DefineEnumNames(clss,enm,deflen,prettynm) \
EnumRef clss::enm##Ref( enm& _e_ ) \
    { return EnumRef( (int&)_e_, enm##Def_ ); } \
const EnumDef& clss::enm##Def() \
    { return enm##Def_; } \
const EnumDef clss::enm##Def_ \
	( prettynm, clss::enm##Names, deflen ); \
const char** clss::enm##NamesGet() \
    { return enm##Names; }  \
const char* clss::enm##Names[] =

inline EnumRef&	EnumRef::operator=( const char* s )
{
    if ( s )
    {
	int i = def_.convert(s);
	if ( i || caseInsensitiveEqual(s,def_.names[0],def_.nrsign) )
	    val = i;
    }
    return *this;
}



#define eString(enm,vr)	(enm##Def().convert((int)vr))
//!< this is the actual enum -> string
#define eEnum(enm,str)	((enm)enm##Def().convert(str))
//!< this is the actual string -> enum
#define eKey(enm)	(enm##Def().name())
//!< this is the 'pretty name' of the enum
#define eRef(enm,vr)	(enm##Ref(vr))

#endif /* #ifndef __cpp__ */


#endif
