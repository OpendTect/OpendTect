#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "factory.h"
#include "transl.h"

class IOObj;
class CtxtIOObj;
class IOObjContext;
class uiLabel;
class uiComboBox;


/*!\brief Group for editing output translator options */

mExpClass(uiIo) uiIOObjTranslatorWriteOpts : public uiGroup
{
public:

			uiIOObjTranslatorWriteOpts(uiParent*,const Translator&);

    mDefineFactory1ParamInClasswKW( uiIOObjTranslatorWriteOpts, uiParent*,
			factory, transl_.getDisplayName() )

    static bool		isPresent( const Translator& t )
			{ return factory().hasName(t.getDisplayName()); }

    static uiIOObjTranslatorWriteOpts* create( uiParent* p, const Translator& t)
			{ return factory().create(t.getDisplayName(),p); }

    virtual void	use(const IOPar&)	= 0;
    virtual bool	fill(IOPar&) const	= 0;
    virtual const char* errMsg() const		{ return errmsg_; }

    const Translator&	translator() const	{ return transl_; }

    Notifier<uiIOObjTranslatorWriteOpts> suggestedNameAvailble;
    virtual const char* suggestedName() const	{ return ""; }

protected:

    const Translator&	transl_;
    mutable BufferString errmsg_;

public:

    void		suggestedNameChanged(CallBacker*)
					{ suggestedNameAvailble.trigger(); }

};


//! For subclasses of uiIOObjTranslatorWriteOpts

#define mDecluiIOObjTranslatorWriteOptsStdFns(clssnm) \
    virtual void	use(const IOPar&); \
    virtual bool	fill(IOPar&) const; \
 \
    static uiIOObjTranslatorWriteOpts* create( uiParent* p ) \
			{ return new clssnm(p); } \
    static void		initClass()



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

protected:

    IOObjContext&	ctxt_;
    ObjectSet<const Translator> trs_;
    ObjectSet<uiIOObjTranslatorWriteOpts> optflds_;

    uiComboBox*		selfld_;
    uiLabel*		lbl_;

    void		mkSelFld(const CtxtIOObj&,bool);
    int			translIdx() const;
    void		selChg(CallBacker*);
    void		nmAvCB( CallBacker* )
					{ suggestedNameAvailble.trigger(); }
    uiIOObjTranslatorWriteOpts* getCurOptFld() const;

};


