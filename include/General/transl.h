#ifndef transl_h
#define transl_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 Contents:	Translators
 RCS:		$Id: transl.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

A translator is an object specific for a certain storage mechanism coupled with
specific details about e.g. vendor specific file transls. A Translator is member
of a group, e.g. the Grid Translator group. Translator groups have a
description of IOObj context.

@$*/


#include <defobj.h>
#include <conn.h>
#include <selector.h>
#include <ctxtioobj.h>
class ostream;
class IOPar;
class UserIDSet;

#define mDGBKey "dGB"

class Translator : public UserIDObject
		 , public DefObject
{		   isUidAbstractDefObject(Translator)
public:

				Translator( const char* nm = 0,
				    const ClassDef* cd = &StreamConn::classdef )
				: UserIDObject(nm)
				, conndef_(cd)		{}

    static const UserIDObjectSet<Translator>& groups()	{ return groups_; }
    virtual const ClassDefList&	defs() const		= 0;
    virtual Translator*		make(const char*) const	= 0;
    virtual ObjectTypeSelectionFun getSelector() const	= 0;
    virtual IOObjContext	getContext() const	= 0;

    virtual int			implExists(const IOObj*,int forread) const;
    virtual int			implRemovable(const IOObj*) const;
    virtual int			implRemove(const IOObj*) const;

    virtual void		usePar(const IOPar*)	{}
    virtual const UserIDSet*	parSpec(Conn::State) const  { return 0; }
    const ClassDef&		connClassDef() const	{ return *conndef_; }
    int				hasConnDef(const ClassDef&) const; // for groups
    virtual const IOObjContext*	xCtxt() const		{ return 0; }

    static void			dumpGroups(ostream&);
    static Translator*		produce(const char* grp, const char*);
    static const char*		sMiscKey;
    static const char*		sMiscUid;

protected:
    const ClassDef*		conndef_;
    static UserIDObjectSet<Translator>	groups_;

    Translator*			trProd(const char*) const;
};


#define isTranslatorGroup(clss) \
hasFactoryUidConcrete(Ppaste(clss,Translator)); \
Translator* make( const char* nm ) const { return trProd(nm); } \
ObjectTypeSelectionFun getSelector() const \
	{ return Ppaste(clss,Translator)::selector; } \
IOObjContext getContext() const \
	{ return Ppaste(clss,Translator)::ioContext(); } \
static int listid;

#define isTranslator(spec,clss) \
private: \
Translator* make(const char* nm) const \
	{ return Ppaste(clss,Translator)::produce(nm); } \
isUidProducable(Ppaste3(spec,clss,Translator))

#define defineTranslatorGroup(clss,prnm) \
defineConcreteFactory(Ppaste(clss,Translator),prnm); \
int Ppaste(clss,Translator)::listid = \
	Translator::groups_ += (Translator*)new Ppaste(clss,Translator)(prnm)

#define defineTranslator(spec,clss,prnm) \
defineProducable(Ppaste3(spec,clss,Translator),Ppaste(clss,Translator),prnm); \


/*$-*/
#endif
