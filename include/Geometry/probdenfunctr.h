#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    const char*		defExtension() const override	{ return "prdf"; }

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
    virtual ProbDenFunc* readInfo(od_istream&)			= 0;
    virtual bool	 write(const ProbDenFunc&,od_ostream&)	= 0;

    bool		binary_;	//!< default: false

};


mExpClass(Geometry) odProbDenFuncTranslator : public ProbDenFuncTranslator
{				 isTranslator(od,ProbDenFunc)
public:
			mDefEmptyTranslatorConstructor(od,ProbDenFunc)

    ProbDenFunc*	read(od_istream&) override;
    ProbDenFunc*	readInfo(od_istream&) override;
    bool		write(const ProbDenFunc&,od_ostream&) override;

};
