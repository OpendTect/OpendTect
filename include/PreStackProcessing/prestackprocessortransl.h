#ifndef prestackprocessortransl_h
#define prestackprocessortransl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Oct 2008
 RCS:		$Id: prestackprocessortransl.h,v 1.1 2008-10-21 13:31:01 cvskris Exp $
________________________________________________________________________


-*/
 
#include "transl.h"
namespace PreStack { class ProcessManager; }


class PreStackProcTranslatorGroup : public TranslatorGroup
{				      isTranslatorGroup(PreStackProc)
public:
    			mDefEmptyTranslatorGroupConstructor(PreStackProc)

    const char*		defExtension() const		{ return "psp"; }
};


class PreStackProcTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(PreStackProc)

    virtual const char*	read(PreStack::ProcessManager&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const PreStack::ProcessManager&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(PreStack::ProcessManager&,const IOObj*,
	    			 BufferString&);
    static bool		store(const PreStack::ProcessManager&,const IOObj*,
	    		      BufferString&);
};


class dgbPreStackProcTranslator : public PreStackProcTranslator
{			     isTranslator(dgb,PreStackProc)
public:

    			mDefEmptyTranslatorConstructor(dgb,PreStackProc)

    const char*		read(PreStack::ProcessManager&,Conn&);
    const char*		write(const PreStack::ProcessManager&,Conn&);

};


#endif
