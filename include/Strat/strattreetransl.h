#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"
#include "transl.h"

mExpClass(Strat) StratTreeTranslatorGroup : public TranslatorGroup
{			isTranslatorGroup(StratTree)
			mDefEmptyTranslatorGroupConstructor(StratTree)
    const char*		defExtension() const override	{ return "sfw"; }
};

mExpClass(Strat) StratTreeTranslator : public Translator
{ mODTextTranslationClass(StratTreeTranslator)
public:
			mDefEmptyTranslatorBaseConstructor(StratTree)

    bool		implRename(const IOObj*,const char*) const override;
    bool		implRemove(const IOObj*,bool) const override;
    bool		implSetReadOnly(const IOObj*,bool) const override;

    static const char*	associatedFileExt();
    static uiString	sStratTree()
			{ return tr("Stratigraphic Tree"); }
};

mExpClass(Strat) odStratTreeTranslator : public StratTreeTranslator
{			isTranslator(od,StratTree)
			 mDefEmptyTranslatorConstructor(od,StratTree)
};
