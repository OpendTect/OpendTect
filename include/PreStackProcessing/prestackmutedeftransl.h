#ifndef prestackmutedeftransl_h
#define prestackmutedeftransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id$
________________________________________________________________________


-*/
 
#include "prestackprocessingmod.h"
#include "transl.h"
namespace PreStack { class MuteDef; }


mExpClass(PreStackProcessing) MuteDefTranslatorGroup : public TranslatorGroup
{				      isTranslatorGroup(MuteDef)
public:
    			mDefEmptyTranslatorGroupConstructor(MuteDef)

    const char*		defExtension() const		{ return "mute"; }
};


mExpClass(PreStackProcessing) MuteDefTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(MuteDef)

    virtual const char*	read(PreStack::MuteDef&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const PreStack::MuteDef&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(PreStack::MuteDef&,const IOObj*,BufferString&);
    static bool		store(const PreStack::MuteDef&,const IOObj*,
	    		      BufferString&);
};


mExpClass(PreStackProcessing) dgbMuteDefTranslator : public MuteDefTranslator
{			     isTranslator(dgb,MuteDef)
public:

    			mDefEmptyTranslatorConstructor(dgb,MuteDef)

    const char*		read(PreStack::MuteDef&,Conn&);
    const char*		write(const PreStack::MuteDef&,Conn&);

    static const char*	sKeyRefHor() { return "Reference Horizon";  }
    static bool		hasIOPar(int majorversion,int minorversion);
};


#endif

