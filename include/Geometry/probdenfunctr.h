#ifndef probdenfunctr_h
#define probdenfunctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: probdenfunctr.h,v 1.4 2010-02-05 12:08:49 cvsnanne Exp $
________________________________________________________________________

-*/
 
#include "transl.h"

class IOObj;
class ProbDenFunc;


mClass ProbDenFuncTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ProbDenFunc)
public:
    			mDefEmptyTranslatorGroupConstructor(ProbDenFunc)

    const char*		defExtension() const		{ return "pdf"; }

};


mClass ProbDenFuncTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(ProbDenFunc)

    static const char*	key();
    virtual ProbDenFunc* read(const IOObj&)			= 0;
    virtual bool	write(const ProbDenFunc&,const IOObj&)	= 0;

};


mClass dgbProbDenFuncTranslator : public ProbDenFuncTranslator
{				  isTranslator(dgb,ProbDenFunc)
public:
    			mDefEmptyTranslatorConstructor(dgb,ProbDenFunc)

    ProbDenFunc*	read(const IOObj&);
    bool		write(const ProbDenFunc&,const IOObj&);

};


#endif
