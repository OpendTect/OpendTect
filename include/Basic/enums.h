#ifndef enums_H
#define enums_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		4-2-1994; 20-10-1995
 Contents:	Enum <--> string conversion and generalized reference
 RCS:		$Id: enums.h,v 1.2 2000-03-02 15:24:27 bert Exp $
________________________________________________________________________

Some utilities surrounding the often needed enum <-> string table.

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

in myclass.cc:

#include <myclass.h>

DefineEnumNames(MyClass,State,1,"My class state")
	{ "Good", "Bad", "Not very handsome", 0 };
DefineEnumNames(MyClass,Type,0,"My class type")
	{ "Yes", "No", "Not sure", 0 };

Note the '1' in the first one telling the EnumDef that only one character needs
to be matched when converting string -> enum. The '0' in the second indicates
that the entire string must match.

This will expand to (added newlines, removed some superfluous stuff:

class MyClass
{
public:

    enum State	{ Good, Bad, Ugly };
    static EnumRef StateRef( State& _e_ ) { return EnumRef((int&)_e_,StateDef);}
    static const EnumDef StateDef;
    static const char* StateNames[]; 

    enum Type   { Yes, No, Maybe };
    Type type() const { return type_; }
    void setType( Type  _e_) { type_ = _e_; }
    EnumRef   typeRef  () const { return EnumRef( (int&)type_, TypeDef ); }
    static EnumRef TypeRef( Type& _e_ ) { return EnumRef((int&)_e_,TypeDef); }
    static const EnumDef TypeDef;
    static const char* TypeNames[];

protected:

    Type type_;

public: 

};

and, in myclass.cc:

const EnumDef MyClass::StateDef( "Handling state", MyClass::StateNames , 1 );
const char* MyClass::StateNames[] = 
	{ "Good", "Bad", "Not very handsome", 0 };
const EnumDef MyClass::TypeDef( "Decision type", MyClass::TypeNames , 0 );
const char* MyClass::TypeNames[] = 
	{ "Yes", "No", "Not sure", 0 };


-*/


#include <string2.h>

#ifndef __cpp__
    int getEnum(const char*,char** namearr,int startnr,int nr_chars_to_match);
#else

#include <uidobj.h>

extern "C" { int getEnum(const char*,const char**,int,int); }


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
    static EnumRef Ppaste(enm,Ref)( enm& _e_ ) \
    { return EnumRef( (int&)_e_, Ppaste(enm,Def) ); } \
    static const EnumDef Ppaste(enm,Def); \
    static const char* Ppaste(enm,Names)[];

#define DeclareEnumUtilsWithVar(enm,varnm) \
public: \
    enm varnm() const { return Ppaste(varnm,_); } \
    void Ppaste(set,enm)(enm _e_) { Ppaste(varnm,_) = _e_; } \
    EnumRef Ppaste(varnm,Ref)() const \
    { return EnumRef( (int&)Ppaste(varnm,_), Ppaste(enm,Def) ); } \
    DeclareEnumUtils(enm) \
protected: \
    enm Ppaste(varnm,_); \
public:

#define DefineEnumNames(clss,enm,deflen,prettynm) \
const EnumDef clss::Ppaste(enm,Def) \
	( prettynm, clss::Ppaste(enm,Names), deflen ); \
const char* clss::Ppaste(enm,Names)[] =


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



#define eKey(enm)	(Ppaste(enm,Def).name())
#define eString(enm,vr)	(Ppaste(enm,Def).convert((int)vr))
#define eEnum(enm,str)	((enm)Ppaste(enm,Def).convert(str))
#define eRef(enm,vr)	(Ppaste(enm,Ref)(vr))

#endif


#endif
