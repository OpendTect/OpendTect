#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Oct 2008
________________________________________________________________________


-*/
 
#include "prestackprocessingmod.h"
#include "transl.h"

namespace PreStack { class ProcessManager; }

/*!
\brief TranslatorGroup for PreStack processing.
*/

mExpClass(PreStackProcessing) PreStackProcTranslatorGroup :
						public TranslatorGroup
{				      isTranslatorGroup(PreStackProc)
public:
			mDefEmptyTranslatorGroupConstructor(PreStackProc)

    const char*		defExtension() const override	{ return "psp"; }
};


/*!
\brief Translator for PreStack processing.
*/

mExpClass(PreStackProcessing) PreStackProcTranslator : public Translator
{ mODTextTranslationClass(PreStackProcTranslator)
public:
			mDefEmptyTranslatorBaseConstructor(PreStackProc)

    virtual uiString	read(PreStack::ProcessManager&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const PreStack::ProcessManager&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(PreStack::ProcessManager&,const IOObj*,
				 uiString&);
    static bool		store(const PreStack::ProcessManager&,const IOObj*,
			      uiString&);

private:
    static uiString	sSelObjNotPreStackProc();
};


/*!
\brief dgb PreStackProcTranslator
*/

mExpClass(PreStackProcessing) dgbPreStackProcTranslator :
						public PreStackProcTranslator
{ mODTextTranslationClass(dgbPreStackProcTranslator)
			isTranslator(dgb,PreStackProc)
public:

			mDefEmptyTranslatorConstructor(dgb,PreStackProc)

    uiString		read(PreStack::ProcessManager&,Conn&) override;
    uiString		write(const PreStack::ProcessManager&,Conn&) override;

};


