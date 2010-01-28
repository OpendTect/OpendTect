#ifndef probdenfunctr_h
#define probdenfunctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: probdenfunctr.h,v 1.2 2010-01-28 10:21:37 cvsnanne Exp $
________________________________________________________________________

-*/
 
#include "transl.h"

class ArrayNDProbDenFunc;
class IOObj;


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

    static const char*	key()			{ return "ProbDenFunc"; }
    virtual bool	read(ArrayNDProbDenFunc&,const IOObj&)		= 0;
    virtual bool	write(const ArrayNDProbDenFunc&,const IOObj&)	= 0;

};


mClass dgbProbDenFuncTranslator : public ProbDenFuncTranslator
{				  isTranslator(dgb,ProbDenFunc)
public:

    			mDefEmptyTranslatorConstructor(dgb,ProbDenFunc)

    bool		read(ArrayNDProbDenFunc&,const IOObj&);
    bool		write(const ArrayNDProbDenFunc&,const IOObj&);

};


#endif
