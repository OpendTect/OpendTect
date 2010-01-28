#ifndef probdenfunctr_h
#define probdenfunctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: probdenfunctr.h,v 1.1 2010-01-28 09:46:38 cvsnanne Exp $
________________________________________________________________________

-*/
 
#include "transl.h"

class IOObj;
class ArrayNDProbDenFunc;


mClass ProbDenFuncTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ProbDenFunc)
public:
    			mDefEmptyTranslatorGroupConstructor(ProbDenFunc)

    const char*		defExtension() const	{ return "pdf"; }
    static const char*	sKeyProbDenFunc()	{ return "ProbDenFunc"; }
};


mClass ProbDenFuncTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(ProbDenFunc)

    virtual bool	read(ArrayNDProbDenFunc&,const IOObj&)		= 0;
    virtual bool	write(const ArrayNDProbDenFunc&,const IOObj&)	= 0;

};


mClass dgbProbDenFuncTranslator : public ProbDenFuncTranslator
{			     isTranslator(dgb,ProbDenFunc)
public:
    			mDefEmptyTranslatorConstructor(dgb,ProbDenFunc)

    virtual bool	read(ArrayNDProbDenFunc&,const IOObj&);
    virtual bool	write(const ArrayNDProbDenFunc&,const IOObj&);

};


#endif
