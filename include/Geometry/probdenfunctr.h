#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "geometrymod.h"
#include "transl.h"
#include "od_iosfwd.h"

class IOObj;
class ProbDenFunc;


mExpClass(Geometry) ProbDenFuncTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ProbDenFunc)
public:
			mDefEmptyTranslatorGroupConstructor(ProbDenFunc)

    const char*		defExtension() const		{ return "prdf"; }

};


mExpClass(Geometry) ProbDenFuncTranslator : public Translator
{ mODTextTranslationClass(ProbDenFuncTranslator);
public:
			ProbDenFuncTranslator(const char* nm,const char* unm);

    static ProbDenFunc* read(const IOObj&,uiString* emsg=0);
    static ProbDenFunc* readInfo(const IOObj&,uiString* emsg=0);
    static bool		write(const ProbDenFunc&,const IOObj&,
			      uiString* emsg=0);

    virtual ProbDenFunc* read(od_istream&)			= 0;
    virtual bool	 write(const ProbDenFunc&,od_ostream&) = 0;;

    bool		binary_;	//!< default: false

};


mExpClass(Geometry) odProbDenFuncTranslator : public ProbDenFuncTranslator
{				 isTranslator(od,ProbDenFunc)
public:
			mDefEmptyTranslatorConstructor(od,ProbDenFunc)

    ProbDenFunc*	read(od_istream&);
    ProbDenFunc*	readInfo(od_istream&);
    bool		write(const ProbDenFunc&,od_ostream&);

};


