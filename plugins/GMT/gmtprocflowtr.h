#ifndef gmtprocflowtr_h
#define gmtprocflowtr_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
 * ID       : $Id: gmtprocflowtr.h,v 1.2 2009-04-06 07:19:31 cvsranojay Exp $
-*/
 
#include "transl.h"
class Conn;
class BufferString;
namespace ODGMT { class ProcFlow; }


mClass ODGMTProcFlowTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ODGMTProcFlow)
public:
    			mDefEmptyTranslatorGroupConstructor(ODGMTProcFlow)

    const char*		defExtension() const		{ return "gmf"; }
};


mClass ODGMTProcFlowTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(ODGMTProcFlow)

    virtual const char*	read(ODGMT::ProcFlow&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const ODGMT::ProcFlow&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(ODGMT::ProcFlow&,const IOObj*,BufferString&);
    static bool		store(const ODGMT::ProcFlow&,const IOObj*,
	    		      BufferString&);

};


mClass dgbODGMTProcFlowTranslator : public ODGMTProcFlowTranslator
{			     isTranslator(dgb,ODGMTProcFlow)
public:

    			mDefEmptyTranslatorConstructor(dgb,ODGMTProcFlow)

    const char*		read(ODGMT::ProcFlow&,Conn&);
    const char*		write(const ODGMT::ProcFlow&,Conn&);

};


#endif
