#ifndef probdenfunctr_h
#define probdenfunctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: probdenfunctr.h,v 1.6 2010-02-09 11:09:19 cvsnanne Exp $
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
    			ProbDenFuncTranslator(const char* nm,const char* unm);

    static const char*	key();
    virtual ProbDenFunc* read(const IOObj&)			= 0;
    virtual bool	write(const ProbDenFunc&,const IOObj&)	= 0;

    bool		binary_;

};


mClass odProbDenFuncTranslator : public ProbDenFuncTranslator
{				 isTranslator(od,ProbDenFunc)
public:
    			mDefEmptyTranslatorConstructor(od,ProbDenFunc)

    ProbDenFunc*	read(const IOObj&);
    bool		write(const ProbDenFunc&,const IOObj&);

};


#endif
