#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

 
#include "prestackprocessingmod.h"
#include "transl.h"

namespace PreStack { class MuteDef; }

/*!
\brief TranslatorGroup for mute definition.
*/

mExpClass(PreStackProcessing) MuteDefTranslatorGroup : public TranslatorGroup
{				      isTranslatorGroup(MuteDef)
public:
			mDefEmptyTranslatorGroupConstructor(MuteDef)

    const char*		defExtension() const override	{ return "mute"; }
};


/*!
\brief Translator for mute definition.
*/

mExpClass(PreStackProcessing) MuteDefTranslator : public Translator
{ mODTextTranslationClass(MuteDefTranslator)
public:
			mDefEmptyTranslatorBaseConstructor(MuteDef)

    virtual const char* read(PreStack::MuteDef&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char* write(const PreStack::MuteDef&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(PreStack::MuteDef&,const IOObj*,uiString&);
    static bool		store(const PreStack::MuteDef&,const IOObj*,
			      uiString&);

private:
    static uiString	sSelObjNotMuteDef();
};


/*!
\brief dgb MuteDefTranslator
*/

mExpClass(PreStackProcessing) dgbMuteDefTranslator : public MuteDefTranslator
{			     isTranslator(dgb,MuteDef)
public:

			mDefEmptyTranslatorConstructor(dgb,MuteDef)

    const char*		read(PreStack::MuteDef&,Conn&) override;
    const char*		write(const PreStack::MuteDef&,Conn&) override;

    static const char*	sKeyRefHor() { return "Reference Horizon";  }
    static bool		hasIOPar(int majorversion,int minorversion);
};
