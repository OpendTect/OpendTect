#ifndef transl_h
#define transl_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 Contents:	Translators
 RCS:		$Id: transl.h,v 1.14 2003-09-25 11:05:45 bert Exp $
________________________________________________________________________

A translator is an object specific for a certain storage mechanism coupled with
specific details about e.g. vendor specific file transls. A Translator is member
of a group, e.g. the Grid Translator group. Translator groups have a
description of IOObj context.

@$*/


#include <defobj.h>
#include <streamconn.h>
#include <selector.h>
#include <ctxtioobj.h>
#include <iopar.h>
#include <iosfwd>

class IOPar;
class CallBack;
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

    static UserIDObjectSet<Translator>& groups();
    virtual const ClassDefList&	defs() const		= 0;
    virtual Translator*		make(const char*) const	= 0;
    virtual ObjectTypeSelectionFun getSelector() const	= 0;
    virtual const IOObjContext&	getContext() const	= 0;

    virtual bool		implExists(const IOObj*,int forread) const;
    virtual bool		implReadOnly(const IOObj*) const;
    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
	    					const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

    virtual void		usePar(const IOPar*)	{}
    virtual const UserIDSet*	parSpec(Conn::State) const  { return 0; }
    const ClassDef&		connClassDef() const	{ return *conndef_; }
    bool			hasConnDef(const ClassDef&) const; // for groups
    virtual const IOObjContext*	xCtxt() const		{ return 0; }

    static void			dumpGroups(ostream&);
    static Translator*		produce(const char* grp, const char*);
    virtual void		clearSelHist()		{}
    virtual const char*		defExtension() const	{ return 0; }

protected:

    const ClassDef*		conndef_;

    Translator*			trProd(const char*) const;
    static IOPar&		mkSelHist(const char*);
};


#define isTranslatorGroup(clss) \
hasFactoryUidConcrete( clss##Translator ); \
Translator* make( const char* nm ) const { return trProd(nm); } \
ObjectTypeSelectionFun getSelector() const \
	{ return clss##Translator::selector; } \
const IOObjContext& getContext() const \
	{ return clss##Translator::ioContext(); } \
void clearSelHist() { selhist.clear(); } \
static int listid; \
static IOPar& selhist;

#define isTranslator(spec,clss) \
private: \
Translator* make(const char* nm) const \
	{ return clss##Translator::produce(nm); } \
isUidProducable(spec##clss##Translator)

#define defineTranslatorGroup(clss,prnm) \
defineConcreteFactory(clss##Translator,prnm); \
int clss##Translator::listid = \
	Translator::groups() += (Translator*)new clss##Translator(prnm); \
IOPar& clss##Translator::selhist = mkSelHist(prnm)

#define defineTranslator(spec,clss,prnm) \
defineProducable(spec##clss##Translator,clss##Translator,prnm);


#endif
