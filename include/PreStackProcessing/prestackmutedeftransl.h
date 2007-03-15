#ifndef mutedeftransl_h
#define mutedeftransl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmutedeftransl.h,v 1.1 2007-03-15 17:28:52 cvskris Exp $
________________________________________________________________________


-*/
 
#include "transl.h"
namespace PreStack { class MuteDef; }


class MuteDefTranslatorGroup : public TranslatorGroup
{				      isTranslatorGroup(MuteDef)
public:
    			mDefEmptyTranslatorGroupConstructor(MuteDef)

    const char*		defExtension() const		{ return "mute"; }
};


class MuteDefTranslator : public Translator
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


class dgbMuteDefTranslator : public MuteDefTranslator
{			     isTranslator(dgb,MuteDef)
public:

    			mDefEmptyTranslatorConstructor(dgb,MuteDef)

    const char*		read(PreStack::MuteDef&,Conn&);
    const char*		write(const PreStack::MuteDef&,Conn&);

};


#endif
