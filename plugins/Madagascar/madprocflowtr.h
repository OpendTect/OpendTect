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
{   isTranslatorGroup(ODMadProcFlow);
    mODTextTranslationClass(ODMadProcFlowTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(ODMadProcFlow)

    const char*		defExtension() const		{ return "mpf"; }
};


mExpClass(Madagascar) ODMadProcFlowTranslator : public Translator
{ mODTextTranslationClass(ODMadProcFlowTranslator)
public:
			mDefEmptyTranslatorBaseConstructor(ODMadProcFlow)

    virtual uiString	read(ODMad::ProcFlow&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const ODMad::ProcFlow&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(ODMad::ProcFlow&,const IOObj*,BufferString&);
    static bool		store(const ODMad::ProcFlow&,const IOObj*,
			      BufferString&);
    static bool		retrieve(ODMad::ProcFlow&,const IOObj*,uiString&);
    static bool		store(const ODMad::ProcFlow&,const IOObj*,
			      uiString&);

};


mClass(Madagascar) dgbODMadProcFlowTranslator : public ODMadProcFlowTranslator
{ mODTextTranslationClass(dgbODMadProcFlowTranslator)
  isTranslator(dgb,ODMadProcFlow)
public:

			mDefEmptyTranslatorConstructor(dgb,ODMadProcFlow)

    uiString		read(ODMad::ProcFlow&,Conn&);
    uiString		write(const ODMad::ProcFlow&,Conn&);

};
