#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
{ isTranslatorGroup(PreStackProc)
public:
			PreStackProcTranslatorGroup();

    const char*		defExtension() const override	{ return "psp"; }
    const char*		getSurveyDefaultKey(
					const IOObj* =nullptr) const override;
};


/*!
\brief Translator for PreStack processing.
*/

mExpClass(PreStackProcessing) PreStackProcTranslator : public Translator
{ mODTextTranslationClass(PreStackProcTranslator)
public:
			~PreStackProcTranslator();

    virtual uiString	read(PreStack::ProcessManager&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const PreStack::ProcessManager&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(PreStack::ProcessManager&,const IOObj*,
				 uiString&);
    static bool		store(const PreStack::ProcessManager&,const IOObj*,
			      uiString&);

protected:

			PreStackProcTranslator(const char* nm,const char* unm);
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
			dgbPreStackProcTranslator(const char* nm,
						  const char* unm);

    uiString		read(PreStack::ProcessManager&,Conn&) override;
    uiString		write(const PreStack::ProcessManager&,Conn&) override;

};
