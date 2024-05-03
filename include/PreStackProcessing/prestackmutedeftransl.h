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
{ isTranslatorGroup(MuteDef)
public:
			MuteDefTranslatorGroup();

    const char*		defExtension() const override	{ return "mute"; }
};


/*!
\brief Translator for mute definition.
*/

mExpClass(PreStackProcessing) MuteDefTranslator : public Translator
{ mODTextTranslationClass(MuteDefTranslator)
public:
			~MuteDefTranslator();

    virtual uiString	read(PreStack::MuteDef&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const PreStack::MuteDef&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(PreStack::MuteDef&,const IOObj*,uiString&);
    static bool		store(const PreStack::MuteDef&,const IOObj*,
			      uiString&);

protected:
			MuteDefTranslator(const char* nm,const char* unm);

private:
    static uiString	sSelObjNotMuteDef();
};


/*!
\brief dgb MuteDefTranslator
*/

mExpClass(PreStackProcessing) dgbMuteDefTranslator : public MuteDefTranslator
{ mODTextTranslationClass(dgbMuteDefTranslator)
    isTranslator(dgb,MuteDef)
public:
			dgbMuteDefTranslator(const char* nm,const char* unm);

    uiString		read(PreStack::MuteDef&,Conn&) override;
    uiString		write(const PreStack::MuteDef&,Conn&) override;

    static bool		hasIOPar(int majorversion,int minorversion);
};
