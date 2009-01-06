#ifndef mutedeftransl_h
#define mutedeftransl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedeftransl.h,v 1.2 2009-01-06 06:05:40 cvsranojay Exp $
________________________________________________________________________


-*/
 
#include "transl.h"
namespace PreStack { class MuteDef; }


mClass MuteDefTranslatorGroup : public TranslatorGroup
{				      isTranslatorGroup(MuteDef)
public:
    			mDefEmptyTranslatorGroupConstructor(MuteDef)

    const char*		defExtension() const		{ return "mute"; }
};


mClass MuteDefTranslator : public Translator
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


mClass dgbMuteDefTranslator : public MuteDefTranslator
{			     isTranslator(dgb,MuteDef)
public:

    			mDefEmptyTranslatorConstructor(dgb,MuteDef)

    const char*		read(PreStack::MuteDef&,Conn&);
    const char*		write(const PreStack::MuteDef&,Conn&);

};


#endif
