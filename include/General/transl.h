#ifndef transl_h
#define transl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		21-10-1995
 Contents:	Translators
 RCS:		$Id: transl.h,v 1.19 2004-05-28 15:25:47 bert Exp $
________________________________________________________________________

A translator is an object specific for a certain storage mechanism coupled with
specific details about e.g. vendor specific file transls. A Translator is member
of a group, e.g. the Grid Translator group. Translator groups have a
description of IOObj context.

*/


#include "sets.h"
#include "selector.h"
#include "callback.h"
#include "bufstring.h"
#include "streamconn.h"
#include "ctxtioobj.h"
class IOPar;
class Translator;

#define mDGBKey "dGB"

#define mObjSelUnrelated	0
#define mObjSelRelated		1
#define mObjSelMatch		2
int defaultSelector(const char*,const char*);


/*!\brief Group of Translators. Has a static factory.
 
  A TranslatorGroup represents a number of IO interpreters for a certain
  object type. For example, Horizons have the HorizonTranslatorGroup singleton
  class which creates HorizonTranslator subclass instances.

  You must define two static methods:
  static int selector(const char*);
     -> whether a certain group name matches. Can usually return defaultSelector
  static const IOObjContext& ioContext();
     -> Returns the IO context. See ctxtioobj.h for details.

 */

class TranslatorGroup
{
public:

				TranslatorGroup( const char* clssnm,
						 const char* usrnm )
				: clssname_(clssnm)
				, usrname_(usrnm)
				, selhist_(0)		{}
    virtual			~TranslatorGroup();

    const BufferString&		clssName() const	{ return clssname_; }
    const BufferString&		userName() const	{ return usrname_; }
    Translator*			make(const char*,bool usrnm=true) const;
    const Translator*		getTemplate(const char*,bool usrnm) const;

    const ObjectSet<const Translator>& templates() const { return templs_; }

    virtual const IOObjContext&	ioCtxt() const		= 0;
    virtual int			objSelector(const char*) const = 0;
    				//!< Return value mObjSelUnrelated etc.

    bool			hasConnType(const char*) const;
    virtual const char*		defExtension() const	{ return 0; }
    IOPar&			selHist();
    void			clearSelHist();

    static const ObjectSet<TranslatorGroup>& groups()	{ return getGroups(); }
    static TranslatorGroup&	getGroup(const char* nm,bool usr=true);

    				// Called from macros
    int				add(Translator*);
    static TranslatorGroup&	addGroup(TranslatorGroup*);

protected:

    BufferString		clssname_;
    BufferString		usrname_;
    ObjectSet<const Translator>	templs_;
    IOPar*			selhist_;

    static ObjectSet<TranslatorGroup>& getGroups();

};


/*!\brief I/O Interpreter class for a certain object type.

  Every concept (Well, Seismic data, etc.) should have its own Translator
  base class. Together with the Group, you then get 2 + N classes per concept:
  XxxTranslatorGroup
  XxxTranslator
  yyyXxxTranslator
  The XxxTranslator defines the 'protocol' for reading and writing objects
  of this type. Actual I/O will always be done through a pointer to that
  type.

  Important: use the macros at the end of this header file to declare and
  define every Translator(Group)-related class.
 */

class Translator : public CallBacker
{
public:
    				Translator( const char* nm, const char* unm )
				: typname_(nm)
				, usrname_(unm)
				, group_(0)		{}
    virtual			~Translator()		{}

    const BufferString&		typeName() const	{ return typname_; }
    const BufferString&		userName() const	{ return usrname_; }
    const TranslatorGroup*	group() const		{ return group_; }

    virtual Translator*		getNew() const		= 0;

    virtual bool		implExists(const IOObj*,int forread) const;
    virtual bool		implReadOnly(const IOObj*) const;
    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
	    					const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

    virtual const char*		connType() const;
    virtual void		usePar(const IOPar&)		{}
    virtual const IOPar*	parSpec(Conn::State) const	{ return 0; }
    virtual const IOObjContext*	xCtxt() const			{ return 0; }
    virtual const char*		defExtension() const
    				{ return group_ ? group_->defExtension() : 0; }

    void			setGroup( TranslatorGroup* g )	{ group_ = g; }

protected:

    BufferString		typname_;
    BufferString		usrname_;
    TranslatorGroup*		group_;

};


// Essential macros for implementing the concept

  // In the class definition:
#define isTranslatorGroup(clss) \
public: \
    static TranslatorGroup& theInst() { return theinst_; } \
    static int selector(const char*); \
    static const IOObjContext& ioContext(); \
    virtual const IOObjContext&	ioCtxt() const { return ioContext(); } \
    virtual int	objSelector( const char* s ) const { return selector(s); } \
private: \
    static TranslatorGroup& theinst_;


#define isTranslator(spec,clss) \
public: \
    Translator* getNew() const \
    { \
	Translator* tr = new spec##clss##Translator(typeName(),userName()); \
	tr->setGroup( group_ ); return tr; \
    } \
    static spec##clss##Translator* getInstance(); \
    static const char* translKey(); \
    static int listID()	{ return listid_; } \
private: \
    static int			listid_;

  // In a source file:
#define defineTranslatorGroup(clss,usrnm) \
TranslatorGroup& clss##TranslatorGroup::theinst_ \
    = TranslatorGroup::addGroup( new clss##TranslatorGroup(#clss,usrnm) );


#define defineTranslator(spec,clss,usrnm) \
int spec##clss##Translator::listid_ \
    = TranslatorGroup::getGroup( #clss , false ).add( \
	    new spec##clss##Translator( #spec, usrnm ) ); \
spec##clss##Translator* spec##clss##Translator::getInstance() \
{ return new spec##clss##Translator(#clss,usrnm); } \
const char* spec##clss##Translator::translKey() { return usrnm; }


// Convenience macros when creating Translator(Group)-related classes
#define mDefEmptyTranslatorGroupConstructor(clss) \
	clss##TranslatorGroup( const char* nm, const char* unm ) \
    	: TranslatorGroup(nm,unm)		{}

#define mDefEmptyTranslatorBaseConstructor(clss) \
	clss##Translator( const char* nm, const char* unm ) \
    	: Translator(nm,unm)			{}

#define mDefEmptyTranslatorConstructor(spec,clss) \
	spec##clss##Translator( const char* nm, const char* unm ) \
    	: clss##Translator(nm,unm)		{}

// Convenience macros when using Translator(Group)-related classes
#define mMkCtxtIOObj(clss) \
	new CtxtIOObj(clss##TranslatorGroup::ioContext())

#define mSelHist(clss) \
	clss##TranslatorGroup::theInst().selHist()

#define mTranslCreate(clss,nm) \
	(clss##Translator*)clss##TranslatorGroup::theInst().make(nm)

#define mTranslGroupName(clss) \
	clss##TranslatorGroup::theInst().userName()

#define mTranslKey(clss) \
    	clss##Translator::translKey()


#endif
