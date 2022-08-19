#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtmod.h"
#include "transl.h"
class Conn;
namespace ODGMT { class ProcFlow; }


mExpClass(GMT) ODGMTProcFlowTranslatorGroup : public TranslatorGroup
{   mTextTranslationClass(ODGMTProcFlowTranslatorGroup,
	    ODGMTProcFlowTranslatorGroup::theInst().translationApplication() )
    isTranslatorGroupBody(ODGMTProcFlow);
public:
			mDefEmptyTranslatorGroupConstructor(ODGMTProcFlow)

    const char*		defExtension() const override	{ return "gmf"; }
};


mExpClass(GMT) ODGMTProcFlowTranslator : public Translator
{ mODTextTranslationClass(ODGMTProcFlowTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(ODGMTProcFlow)

    virtual uiString	read(ODGMT::ProcFlow&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const ODGMT::ProcFlow&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(ODGMT::ProcFlow&,const IOObj*,uiString&);
    static bool		store(const ODGMT::ProcFlow&,const IOObj*,uiString&);

};


mExpClass(GMT) dgbODGMTProcFlowTranslator : public ODGMTProcFlowTranslator
{     isTranslator(dgb,ODGMTProcFlow);
      mODTextTranslationClass(dgbODGMTProcFlowTranslator);
public:

			mDefEmptyTranslatorConstructor(dgb,ODGMTProcFlow)

    uiString		read(ODGMT::ProcFlow&,Conn&) override;
    uiString		write(const ODGMT::ProcFlow&,Conn&) override;

};
