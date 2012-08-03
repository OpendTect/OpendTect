#ifndef madprocflowtr_h
#define madprocflowtr_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2007
 * ID       : $Id: madprocflowtr.h,v 1.4 2012-08-03 13:01:32 cvskris Exp $
-*/
 
#include "madagascarmod.h"
#include "transl.h"
class Conn;
class BufferString;
namespace ODMad { class ProcFlow; }


mClass(Madagascar) ODMadProcFlowTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ODMadProcFlow)
public:
    			mDefEmptyTranslatorGroupConstructor(ODMadProcFlow)

    const char*		defExtension() const		{ return "mpf"; }
};


mClass(Madagascar) ODMadProcFlowTranslator : public Translator
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

