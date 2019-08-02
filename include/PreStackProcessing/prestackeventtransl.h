#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "transl.h"

class Executor;
class IOObj;
class BinnedValueSet;
class TrcKeySampling;

namespace PreStack { class EventManager; }

/*!
\brief TranslatorGroup for PreStack Event.
*/

mExpClass(PreStackProcessing) PSEventTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(PSEvent);
    mODTextTranslationClass(PSEventTranslatorGroup);;
public:
				mDefEmptyTranslatorGroupConstructor(PSEvent);
    const char*			defExtension() const { return sDefExtension(); }
    static const char*		sDefExtension()	     { return "psevent"; }
};


/*!
\brief Translator for PreStack Event.
*/

mExpClass(PreStackProcessing) PSEventTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(PSEvent);
    virtual Executor*	createReader(PreStack::EventManager&,
				     const BinnedValueSet*,
				     const TrcKeySampling*,IOObj*,
				     bool trigger)	= 0;
    virtual Executor*	createWriter(PreStack::EventManager&,IOObj*) = 0;
    virtual Executor*	createSaveAs(PreStack::EventManager&,IOObj*)	= 0;
    virtual Executor*	createOptimizer(IOObj*)				= 0;

    static Executor*	reader(PreStack::EventManager&, const BinnedValueSet*,
			       const TrcKeySampling*, IOObj*, bool trigger );
    static Executor*	writer(PreStack::EventManager&,IOObj*);
    static Executor*	writeAs(PreStack::EventManager&,IOObj*);
};


/*!
\brief dgb PSEventTranslator.
*/

mExpClass(PreStackProcessing) dgbPSEventTranslator : public PSEventTranslator
{ isTranslator(dgb,PSEvent)
public:
			mDefEmptyTranslatorConstructor(dgb,PSEvent);
    Executor*		createReader(PreStack::EventManager&,
				     const BinnedValueSet*,
				     const TrcKeySampling*,IOObj*,bool);
    Executor*		createWriter(PreStack::EventManager&,IOObj*);
    Executor*		createSaveAs(PreStack::EventManager&,IOObj*);
    Executor*		createOptimizer(IOObj*) { return 0; }

};
