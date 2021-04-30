#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2007
-*/

#include "madagascarmod.h"
#include "transl.h"
class Conn;
namespace ODMad { class ProcFlow; }


mExpClass(Madagascar) ODMadProcFlowTranslatorGroup : public TranslatorGroup
{   mTextTranslationClass(ODMadProcFlowTranslatorGroup,
	    ODMadProcFlowTranslatorGroup::theInst().translationApplication() )
    isTranslatorGroupBody(ODMadProcFlow);
public:
			mDefEmptyTranslatorGroupConstructor(ODMadProcFlow)

    const char*		defExtension() const		{ return "mpf"; }
};


mExpClass(Madagascar) ODMadProcFlowTranslator : public Translator
{ mODTextTranslationClass(ODMadProcFlowTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(ODMadProcFlow)

    virtual uiString	read(ODMad::ProcFlow&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const ODMad::ProcFlow&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(ODMad::ProcFlow&,const IOObj*,uiString&);
    static bool		store(const ODMad::ProcFlow&,const IOObj*,uiString&);

};


mClass(Madagascar) dgbODMadProcFlowTranslator : public ODMadProcFlowTranslator
{     isTranslator(dgb,ODMadProcFlow);
      mODTextTranslationClass(dgbODMadProcFlowTranslator);
public:

			mDefEmptyTranslatorConstructor(dgb,ODMadProcFlow)

    uiString		read(ODMad::ProcFlow&,Conn&) override;
    uiString		write(const ODMad::ProcFlow&,Conn&) override;

};


