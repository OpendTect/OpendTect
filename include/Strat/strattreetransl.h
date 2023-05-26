#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"
#include "transl.h"

mDeclEmptyTranslatorBundle(Strat,StratTree,od,"sfw")

mExpClass(Strat) StratTreeNTranslatorGroup : public TranslatorGroup
{			isTranslatorGroup(StratTreeN)
			mDefEmptyTranslatorGroupConstructor(StratTreeN)
    const char*		defExtension() const override	{ return "sfw"; }
};

mExpClass(Strat) StratTreeNTranslator : public Translator
{ mODTextTranslationClass(StratTreeNTranslator)
public:
			mDefEmptyTranslatorBaseConstructor(StratTreeN)

    bool		implRename(const IOObj*,const char*) const override;
    bool		implRemove(const IOObj*,bool) const override;
    bool		implSetReadOnly(const IOObj*,bool) const override;

    static const char*	associatedFileExt();
    static uiString	sStratTree()
			{ return tr("Stratigraphic Tree"); }
};

mExpClass(Strat) odStratTreeNTranslator : public StratTreeNTranslator
{			isTranslator(od,StratTreeN)
			 mDefEmptyTranslatorConstructor(od,StratTreeN)
};
