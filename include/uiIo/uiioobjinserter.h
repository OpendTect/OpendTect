#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "factory.h"
#include "transl.h"

class uiButton;
class uiParent;
class uiToolButtonSetup;


/*!\brief inserts a new object into the OD data store.

The initial idea is to use it for external objects that are not imported. In
time though, they could make 'import' menus (almost) obsolete.

 */

mExpClass(uiIo) uiIOObjInserter : public CallBacker
{
public:
			uiIOObjInserter(const Translator&);
    virtual		~uiIOObjInserter();

    mDefineFactoryInClasswKW( uiIOObjInserter, factory, factoryName() )

    const char*		name() const	    { return transl_.userName(); }
    const char*		factoryName() const { return transl_.getDisplayName(); }

    static bool		allDisabled();
    bool		isDisabled() const;

    static bool		isPresent(const TranslatorGroup&);
    static bool		isPresent( const Translator& t )
			{ return factory().hasName(t.getDisplayName()); }

    static void		addInsertersToDlg(uiParent*,CtxtIOObj&,
					  ObjectSet<uiIOObjInserter>&,
					  ObjectSet<uiButton>&,
					  const BufferStringSet&);
    static uiIOObjInserter* create( const Translator& t )
			{ return factory().create(t.getDisplayName()); }

    virtual uiToolButtonSetup*	getButtonSetup() const		= 0;
    bool		hasTranslator( const Translator& trl ) const
			{ return &transl_ == &trl; }

    void		setIOObjCtxt(const IOObjContext&);

    CNotifier<uiIOObjInserter,const MultiID&> objInserterd;

protected:

    const Translator&	transl_;
    IOObjContext&	ctxt_;
};
