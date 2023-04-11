#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "callback.h"
#include "refcount.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "objectset.h"
#include "streamconn.h"
#include "uistring.h"

/*
  A translator is an object specific for a certain storage mechanism coupled
  with specific details about how to interpret/write a certain format.

  A Translator is member of a group, e.g. the Wavelet Translator group.
  Translator groups have a description of IOObj context needed to select an
  object of that type.
*/


class Translator;

#define mDGBKey "dGB"
#define mdTectKey "dTect"

#define mObjSelUnrelated	0
#define mObjSelRelated		1
#define mObjSelMatch		2
mGlobal(General) int defaultSelector(const char*,const char*);


/*!\brief Group of Translators. Has a static factory.

  A TranslatorGroup represents a number of IO interpreters for a certain
  object type. For example, Wavelets have the WaveletTranslatorGroup singleton
  class which creates WaveletTranslator subclass instances.

  You must define two static methods:
  static int selector(const char*);
     -> whether a certain group name matches. Can usually return defaultSelector
  static const IOObjContext& ioContext();
     -> Returns the IO context. See ioobjctxt.h for details.

 */

mExpClass(General) TranslatorGroup : public ReferencedObject
{
public:
    virtual StringView		groupName() const = 0;
    virtual uiString		typeName(int num=1) const = 0;
    virtual Translator*		make(const char*,bool usrnm) const;
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
    static TranslatorGroup&	getGroup(const char* nm);
    static bool			hasGroup(const char* nm);
    static void			clearSelHists();

				// Called from macros
    bool			add(Translator*);
    static TranslatorGroup&	addGroup(TranslatorGroup*);

    int				defTranslIdx() const	{ return deftridx_; }
    void			setDefTranslIdx( int i ) { deftridx_ = i; }

    virtual const char*		getSurveyDefaultKey(const IOObj* = 0) const;

    virtual const char*		translationApplication() const;

protected:
				TranslatorGroup(const char* clssnm);
    virtual			~TranslatorGroup();

    const OD::String&		clssName() const	{ return clssname_; }

    BufferString		clssname_;
    ObjectSet<const Translator>	templs_;
    int				deftridx_		= 0;
    IOPar*			selhist_		= nullptr;

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

mExpClass(General) Translator : public CallBacker
{ mODTextTranslationClass(Translator)
public:
				Translator(const char* nm,const char* usr_nm);
    virtual			~Translator()		{}

    const OD::String&		typeName() const	{ return typname_; }
    const OD::String&		userName() const	{ return usrname_; }
    const TranslatorGroup*	group() const		{ return group_; }

    virtual Translator*		getNew() const		= 0;

    virtual bool		implIsLink(const IOObj*) const;
    virtual bool		implExists(const IOObj*,bool forread) const;
    virtual bool		implReadOnly(const IOObj*) const;
    virtual bool		implRename(const IOObj*,
					   const char* newnm) const;
    virtual bool		implRemove(const IOObj*,bool deep=true) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

    virtual const char*		connType() const;
    virtual void		usePar(const IOPar&)		{}
    virtual const char*		defExtension() const;

    void			setGroup( TranslatorGroup* g )	{ group_ = g; }

    virtual bool		isUserSelectable( bool forread=true ) const
				{ return true; }
    virtual bool		getConfirmRemoveMsg(const IOObj*,uiString& msg,
					uiString& canceltxt,
					uiString& deepremovetxt,
					uiString& shallowremovetxt) const;
				/*!< if deepremove option is selected by user,
				     the implRemove call gets deep=true */

    virtual IOObj*		createWriteIOObj(const IOObjContext&,
						 const MultiID&) const;
    virtual const char*		iconName() const	{ return "od"; }
    virtual const uiString&	message() const		{ return msg_; }

    const char*			getDisplayName() const; //!< "username [group]"
    static const Translator*	getTemplateInstance(const char* displayname);

protected:

    BufferString		typname_;
    BufferString		usrname_;
    uiString			msg_;

    TranslatorGroup*		group_			= nullptr;

    static BufferString		getAssociatedFileName(const IOObj&,
						const char* extension);
    static BufferString		getAssociatedFileName(const char* fnm,
						const char* extension);
    static bool			renameAssociatedFile(const char* fnm,
				    const char* ext,const char* newnm);
    static bool			removeAssociatedFile(const char* fnm,
				    const char* ext);
    static bool			setPermAssociatedFile(const char* fnm,
				    const char* ext,bool setwritable);
    static bool			renameLargeFile(const char* fnm,
						const char* newfnm,
						const CallBack* cb=nullptr);

public:

    static uiString		sNoIoobjMsg();
    static uiString		sBadConnection();
    uiString			sSelObjectIsWrongType();

};




// Essential macros for implementing the concept
#define mImplTranslatorInitClass( spec, clss, usrnm ) \
{ \
    spec##clss##Translator* tr = new spec##clss##Translator( #spec, usrnm ); \
    clss##TranslatorGroup::theInst().add( tr ); \
}

#define mImplTranslatorGroupTheInst( clss ) \
{ \
    mDefineStaticLocalObject(RefMan<TranslatorGroup>, inst, \
	= &TranslatorGroup::addGroup( new clss##TranslatorGroup ));\
    return *inst; \
}


//! In the class definition of a TranslatorGroup class
#define isTranslatorGroupBody(clss) \
protected: \
    ~clss##TranslatorGroup() {} \
public: \
    static int selector(const char*); \
    static void initClass() { theInst(); } \
    static const IOObjContext& ioContext(); \
    static StringView sGroupName(); \
    static uiString sTypeName(int num=1); \
    StringView groupName() const override { return sGroupName(); } \
    uiString typeName(int num=1) const override\
    { return sTypeName(num); } \
    const IOObjContext& ioCtxt() const override { return ioContext(); } \
    int objSelector( const char* s ) const override { return selector(s); } \
    static TranslatorGroup& theInst()


#define isTranslatorGroup( clss ) \
mTextTranslationClass( clss##TranslatorGroup, \
	       clss##TranslatorGroup::theInst().translationApplication() ) \
isTranslatorGroupBody(clss);

#define isTranslatorGroupWithInst( clss ) \
isTranslatorGroupBody( clss ) \
mImplTranslatorGroupTheInst( clss )


  //! In the class definition of a Translator class
#define isTranslatorBody(spec,clss) \
public: \
    Translator* getNew() const override \
    { \
	Translator* trans = new spec##clss##Translator(typeName().buf(), \
						    userName().buf()); \
	trans->setGroup( group_ ); return trans; \
    } \
    static spec##clss##Translator* getInstance(); \
    static const char* translKey(); \
    static void initClass()


#define isTranslator( spec, clss ) \
isTranslatorBody( spec, clss );

#define mIsTranslatorWithInitClass( spec, clss, usrnm ) \
isTranslatorBody(spec, clss) \
mImplTranslatorInitClass(spec, clss, usrnm )

  //! In the source file of a TranslatorGroup class
#define defineTranslatorGroup(clss,groupname) \
StringView clss##TranslatorGroup::sGroupName() \
{ return groupname; } \
TranslatorGroup& clss##TranslatorGroup::theInst() \
mImplTranslatorGroupTheInst( clss )

//! In the source file of a Translator class
#define defineTranslator(spec,clss,usrnm) \
spec##clss##Translator* spec##clss##Translator::getInstance() \
{ return new spec##clss##Translator(#clss,usrnm); } \
const char* spec##clss##Translator::translKey() { return usrnm; } \
void spec##clss##Translator::initClass() \
mImplTranslatorInitClass( spec, clss, usrnm )


  //! Convenience when the TranslatorGroup is not interesting 4 u.
  //! Defines a simple empty TranslatorGroup class body
#define mDefEmptyTranslatorGroupConstructor(clss) \
	clss##TranslatorGroup() \
	: TranslatorGroup(#clss)		{} \
	mOD_DisableCopy(clss##TranslatorGroup)

  //! Convenience when the Translator base class is not interesting 4 u.
  //! Defines a simple empty Translator class body
#define mDefEmptyTranslatorBaseConstructor(clss) \
	clss##Translator( const char* nm, const char* unm ) \
	: Translator(nm,unm)			{} \
	mOD_DisableCopy(clss##Translator)

  //! Convenience when the Translator is not interesting 4 u.
  //! Defines a simple empty Translator subclass body
#define mDefEmptyTranslatorConstructor(spec,clss) \
	spec##clss##Translator( const char* nm, const char* unm ) \
	: clss##Translator(nm,unm)		{} \
	mOD_DisableCopy(spec##clss##Translator)

  //! Convenient when the entire Translator concept is not interesting 4 u.
  //! Use this in your header file to comply with the concept, so you
  //! can make use of OpendTect object selection, retrieval etc.
#define mDeclEmptyTranslatorBundle(mod,clss,fmt,defext) \
mExpClass(mod) clss##TranslatorGroup : public TranslatorGroup \
{			isTranslatorGroup(clss) \
			mDefEmptyTranslatorGroupConstructor(clss) \
    const char*		defExtension() const override	{ return defext; } \
}; \
 \
mExpClass(mod) clss##Translator : public Translator \
{ public: \
			mDefEmptyTranslatorBaseConstructor(clss) \
}; \
 \
mExpClass(mod) fmt##clss##Translator : public clss##Translator \
{			isTranslator(fmt,clss) \
			mDefEmptyTranslatorConstructor(fmt,clss) \
};

  //! Definitions for .cc file:
  //! Convenience when the entire Translator concept is not interesting
  //! This one can be convenient when a group has only one Translator
#define mDefSimpleTranslatorInstances(clss,usrnm,fmt) \
defineTranslatorGroup(clss,usrnm) \
uiString clss##TranslatorGroup::sTypeName(int) { return toUiString(usrnm); } \
defineTranslator(fmt,clss,#fmt)

  //! Definitions for .cc file:
  //! This particular macro defines a simple object selection definition
#define mDefSimpleTranslatorSelector(clss) \
int clss##TranslatorGroup::selector( const char* s ) \
{ return defaultSelector(clss##TranslatorGroup::sGroupName(),s); }

  //! Definitions for .cc file:
  //! Defines the function providing the IOObj context
  //! This particular macro allows extension, setting extra members
#define mDefSimpleTranslatorioContextWithExtra(clss,stdtyp,extra) \
const IOObjContext& clss##TranslatorGroup::ioContext() \
{ \
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ctxt, = nullptr ); \
    if ( !ctxt ) \
    { \
	ctxt = new IOObjContext( nullptr ); \
	ctxt->stdseltype_ = IOObjContext::stdtyp; \
	extra; \
    } \
    ctxt->trgroup_ = &theInst(); \
    return *ctxt; \
}

  //! Definitions for .cc file:
  //! Defines the function providing the IOObj context, no extras
#define mDefSimpleTranslatorioContext(clss,stdtyp) \
    mDefSimpleTranslatorioContextWithExtra(clss,stdtyp,)

  //! Definitions for .cc file:
  //! Defines all necessary functions to implement
  //! This is the 'basic' one
#define mDefSimpleTranslators(clss,usrnm,fmt,stdtyp) \
mDefSimpleTranslatorSelector(clss)\
mDefSimpleTranslatorInstances(clss,usrnm,fmt) \
mDefSimpleTranslatorioContext(clss,stdtyp)

  //! Definitions for .cc file:
  //! Defines all necessary functions to implement
  //! This one allows adding code to set IOObjContext members
#define mDefSimpleTranslatorsWithCtioExtra(clss,usrnm,fmt,stdtyp,extra) \
mDefSimpleTranslatorInstances(clss,usrnm,fmt) \
mDefSimpleTranslatorSelector(clss) \
mDefSimpleTranslatorioContextWithExtra(clss,stdtyp,extra)

  //! Definitions for .cc file:
  //! Defines all necessary functions to implement
  //! This one sets the selkey
  //! Therefore it's the one you want to use if you have your own data sub-dir.
#define mDefSimpleTranslatorsWithSelKey(clss,usrnm,fmt,stdtyp,selky) \
    mDefSimpleTranslatorsWithCtioExtra(clss,usrnm,fmt,stdtyp, \
		ctxt->selkey_ = selky)

  //! Definitions for .cc file:
  //! Convenient when the entire Translator concept is not interesting 4 u.
  //! defines one 'format'
  //! Example: mDefEmptyTranslatorBundle( MyConfig, myFmt, Misc, "MyFmt",
  //!          "My Format", MyConfigMgr::getUiConfigStr )
#define mDefEmptyTranslatorBundle(clss,fmt,stdtyp,fmtky,usrnm,uistrfn) \
    defineTranslatorGroup(clss,usrnm); \
    uiString clss##TranslatorGroup::sTypeName( int num ) \
	{ return uistrfn(); } \
    defineTranslator(fmt,clss,fmtky); \
    mDefSimpleTranslatorioContext(clss,stdtyp); \
    mDefSimpleTranslatorSelector(clss)

// Convenience macros when using Translator(Group)-related classes

#define mIOObjContext(trgrp) \
	trgrp##TranslatorGroup::ioContext()

#define mTranslGroupName(trgrp) \
	trgrp##TranslatorGroup::sGroupName()

#define mTranslTemplInstance(trgrp,tr) \
	(*trgrp##TranslatorGroup::theInst() \
		.getTemplate(tr##trgrp##Translator::translKey(),true))

#define mTranslCreate(trgrp,tr) \
	(trgrp##Translator*)trgrp##TranslatorGroup::theInst().make(tr)

#define mTranslKey(trgrp,tr) \
	tr##trgrp##Translator::translKey()

#define mMkCtxtIOObj(trgrp) \
	new CtxtIOObj(trgrp##TranslatorGroup::ioContext())

#define mSelHist(trgrp) \
	trgrp##TranslatorGroup::theInst().selHist()
