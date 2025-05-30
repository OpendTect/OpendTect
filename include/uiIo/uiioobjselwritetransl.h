#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "factory.h"
#include "transl.h"

class CtxtIOObj;
class IOObj;
class IOObjContext;
class uiLabel;
class uiComboBox;


/*!\brief Group for editing output translator options */

mExpClass(uiIo) uiIOObjTranslatorWriteOpts : public uiGroup
{
public:
			~uiIOObjTranslatorWriteOpts();

    mDefineFactory1ParamInClasswKW( uiIOObjTranslatorWriteOpts, uiParent*,
				    factory, transl_.getDisplayName() )

    static bool		isPresent( const Translator& t )
			{ return factory().hasName(t.getDisplayName()); }

    static uiIOObjTranslatorWriteOpts* create( uiParent* p, const Translator& t)
			{ return factory().create(t.getDisplayName(),p); }

    virtual void	useObj(const IOObj&);
    virtual void	usePar(const IOPar&)	= 0;
    virtual bool	fillObj(IOObj&) const;
    virtual bool	fillPar(IOPar&) const	= 0;
    virtual const char* errMsg() const		{ return errmsg_; }

    const Translator&	translator() const	{ return transl_; }

    Notifier<uiIOObjTranslatorWriteOpts> suggestedNameAvailble;
    virtual const char* suggestedName() const	{ return ""; }

protected:
			uiIOObjTranslatorWriteOpts(uiParent*,const Translator&);

    const Translator&	transl_;
    mutable BufferString errmsg_;

public:

    void		suggestedNameChanged(CallBacker*)
					{ suggestedNameAvailble.trigger(); }

    mDeprecated("Use usePar")
    virtual void	use( const IOPar& par )
			{ usePar( par ); }
    mDeprecated("Use fillPar")
    virtual bool	fill( IOPar& par ) const
			{ return fillPar( par ); }

};


/*!\brief Group for selecting output translator */

mExpClass(uiIo) uiIOObjSelWriteTranslator : public uiGroup
{ mODTextTranslationClass(uiIOObjSelWriteTranslator)
public:
			uiIOObjSelWriteTranslator(uiParent*,const CtxtIOObj&,
				const BufferStringSet& transltoavoid,
				bool withopts=false);
			~uiIOObjSelWriteTranslator();

    bool		isEmpty() const;
    void		setTranslator(const Translator*);
    const Translator*	selectedTranslator() const;

    IOObj*		mkEntry(const char*) const;
    void		updatePars(IOObj&) const;
    bool		hasWriteOpts() const	{ return !optflds_.isEmpty(); }
    bool		hasSameWriteOpts(const uiIOObjSelWriteTranslator&);

    void		use(const IOObj&);
    void		resetPars();

    bool		hasSelectedTranslator(const IOObj&) const;

    Notifier<uiIOObjSelWriteTranslator> suggestedNameAvailble;
    virtual const char* suggestedName() const;

    void		updateTransFld(const BufferStringSet& transltoavoid);

protected:

    IOObjContext&			    ctxt_;
    ObjectSet<const Translator>		    trs_;
    ObjectSet<uiIOObjTranslatorWriteOpts>   optflds_;

    uiComboBox*				    selfld_		= nullptr;
    uiLabel*				    lbl_		= nullptr;

    void				mkSelFld(const CtxtIOObj&,bool);
    int					translIdx() const;
    void				selChg(CallBacker*);
    void				nmAvCB( CallBacker* )
					{ suggestedNameAvailble.trigger(); }
    uiIOObjTranslatorWriteOpts*		getCurOptFld() const;

};


//! For subclasses of uiIOObjTranslatorWriteOpts, obsolete macro, do not use

#define mDecluiIOObjTranslatorWriteOptsStdFns(clssnm) \
    void		usePar(const IOPar&) override; \
    bool		fillPar(IOPar&) const override; \
 \
    static uiIOObjTranslatorWriteOpts* create( uiParent* p ) \
			{ return new clssnm(p); } \
    static void		initClass()
