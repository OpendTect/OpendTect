#ifndef enums_H
#define enums_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		4-2-1994; 20-10-1995
 Contents:	Enum <--> string conversion and generalized reference
 RCS:		$Id: enums.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/


#include <string2.h>

#ifndef __cpp__
    int getEnum	Pargs( (const char*,char**,int,int) );
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


/*$@
In normal usage, enum utilities are declared and defined by macros. These
should be placed in the public section of the class.
Example of usage:\line
\line
in myclass.h:
@$
#include <enums.h>

class MyClass
{
public:
    enum State   { Good, Bad, Ugly };
    DeclareEnumUtils(State)
    enum Type   { Yes, No, Maybe };
    DeclareEnumUtilsWithVar(Type,type)

    // rest of class

};
$@
in myclass.cc:
@$
#include <myclass.h>

DefineEnumNames(MyClass,State,1,"My class state")
	{ "Good", "Bad", "Not very handsome", 0 };
DefineEnumNames(MyClass,Type,1,"My class type")
	{ "Yes", "No", "Not sure", 0 };

// rest of implementation
$@
This will expand to (added newlines, removed some superfluous stuff:
@$
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
$@
and, in myclass.cc:
@$
const EnumDef MyClass::StateDef( "Handling state", MyClass::StateNames , 1 );
const char* MyClass::StateNames[] = 
	{ "Good", "Bad", "Not very handsome", 0 };
const EnumDef MyClass::TypeDef( "Decision type", MyClass::TypeNames , 1 );
const char* MyClass::TypeNames[] = 
	{ "Yes", "No", "Not sure", 0 };
@$*/

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


// Most used macros!
#define eKey(enm)	(Ppaste(enm,Def).name())
#define eString(enm,vr)	(Ppaste(enm,Def).convert((int)vr))
#define eEnum(enm,str)	((enm)Ppaste(enm,Def).convert(str))
#define eRef(enm,vr)	(Ppaste(enm,Ref)(vr))

#endif

/*$-*/
#endif
