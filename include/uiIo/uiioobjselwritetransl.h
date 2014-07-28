#ifndef uiioobjselwritetransl_h
#define uiioobjselwritetransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "factory.h"

class IOObj;
class CtxtIOObj;
class Translator;
class IOObjContext;
class uiLabel;
class uiComboBox;


/*!\brief Group for editing output translator options */

mExpClass(uiIo) uiIOObjTranslatorWriteOpts : public uiGroup
{
public:

			uiIOObjTranslatorWriteOpts(uiParent*,const Translator&);

    mDefineFactory1ParamInClasswKW( uiIOObjTranslatorWriteOpts, uiParent*,
			factory, getName4Factory(transl_) )

    static const char*	getName4Factory(const Translator&);
				//! add using this name

    static bool		availableFor( const Translator& t )
			{ return factory().hasName(getName4Factory(t)); }
    static uiIOObjTranslatorWriteOpts* create( uiParent* p, const Translator& t)
			{ return factory().create(getName4Factory(t),p); }

    virtual void	use(const IOPar&)	= 0;
    virtual bool	fill(IOPar&) const	= 0;
    virtual const char*	errMsg() const		{ return errmsg_; }

    const Translator&	translator() const	{ return transl_; }

protected:

    const Translator&	transl_;
    mutable BufferString errmsg_;

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
{
public:
			uiIOObjSelWriteTranslator(uiParent*,const CtxtIOObj&,
					    bool withopts=false);
			~uiIOObjSelWriteTranslator();

    bool		isEmpty() const;
    void		setTranslator(const Translator*);
    const Translator*	selectedTranslator() const;

    IOObj*		mkEntry(const char*) const;
    void		use(const IOObj&);
    void		updatePars(IOObj&) const;

    bool		hasSelectedTranslator(const IOObj&) const;

protected:

    IOObjContext&	ctxt_;
    ObjectSet<const Translator> trs_;
    ObjectSet<uiIOObjTranslatorWriteOpts> optflds_;

    uiComboBox*		selfld_;
    uiLabel*		lbl_;

    void		mkSelFld(const CtxtIOObj&,bool);
    int			translIdx() const;
    void		selChg(CallBacker*);
    uiIOObjTranslatorWriteOpts* getCurOptFld() const;

};


#endif
