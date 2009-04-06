#ifndef madprocflowtr_h
#define madprocflowtr_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Dec 2007
 * ID       : $Id: madprocflowtr.h,v 1.2 2009-04-06 07:24:44 cvsranojay Exp $
-*/
 
#include "transl.h"
class Conn;
class BufferString;
namespace ODMad { class ProcFlow; }


mClass ODMadProcFlowTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ODMadProcFlow)
public:
    			mDefEmptyTranslatorGroupConstructor(ODMadProcFlow)

    const char*		defExtension() const		{ return "mpf"; }
};


mClass ODMadProcFlowTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(ODMadProcFlow)

    virtual const char*	read(ODMad::ProcFlow&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const ODMad::ProcFlow&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(ODMad::ProcFlow&,const IOObj*,BufferString&);
    static bool		store(const ODMad::ProcFlow&,const IOObj*,
	    		      BufferString&);

};


class dgbODMadProcFlowTranslator : public ODMadProcFlowTranslator
{			     isTranslator(dgb,ODMadProcFlow)
public:

    			mDefEmptyTranslatorConstructor(dgb,ODMadProcFlow)

    const char*		read(ODMad::ProcFlow&,Conn&);
    const char*		write(const ODMad::ProcFlow&,Conn&);

};


#endif
