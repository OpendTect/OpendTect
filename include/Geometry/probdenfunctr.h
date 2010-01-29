#ifndef probdenfunctr_h
#define probdenfunctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: probdenfunctr.h,v 1.3 2010-01-29 11:46:34 cvsnanne Exp $
________________________________________________________________________

-*/
 
#include "transl.h"

class IOObj;
class IOPar;
class SampledProbDenFuncND;


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
    virtual bool	read(SampledProbDenFuncND&,const IOObj&)	= 0;
    virtual bool	write(const SampledProbDenFuncND&,const IOObj&)	= 0;

    static const char*	sKeyNrDim();
    static const char*	sKeyDimName();
    static const char*	sKeySize();
    static const char*	sKeySampling();

};


mClass dgbProbDenFuncTranslator : public ProbDenFuncTranslator
{				  isTranslator(dgb,ProbDenFunc)
public:

    			mDefEmptyTranslatorConstructor(dgb,ProbDenFunc)

    bool		read(SampledProbDenFuncND&,const IOObj&);
    bool		write(const SampledProbDenFuncND&,const IOObj&);

    void		fillPar(const SampledProbDenFuncND&,IOPar&);

};


#endif
