#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "transl.h"

class Executor;
class IOObj;
namespace EM { class Body; }

typedef EM::Body	EMBody;

/*!\brief TranslatorGroup for EM::Body.*/

mExpClass(EarthModel) EMBodyTranslatorGroup : public TranslatorGroup
{			isTranslatorGroup(EMBody)
public:
			mDefEmptyTranslatorGroupConstructor(EMBody)

    const char*		defExtension() const override	{ return "body"; }
    static StringView	sKeyExtension()			{ return "body"; }
    static StringView	sKeyUserWord()			{ return "od"; }
};


/*!\brief Base class for all EM::Body Translators */

mExpClass(EarthModel) EMBodyTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(EMBody)

    virtual Executor*	writer(const EM::Body&,IOObj&)		= 0;
    virtual Executor*	reader(const IOObj&)			= 0;
    virtual EMBody*	getReadBody()				= 0;
    virtual uiString	errMsg() const				= 0;
};



/*!\brief OpendTect format EM::Body Translator. */

mExpClass(EarthModel) odEMBodyTranslator : public EMBodyTranslator
{					   isTranslator(od,EMBody)
public:
			odEMBodyTranslator(const char* nm,const char* unm);
			~odEMBodyTranslator();

    Executor*		writer(const EM::Body&,IOObj&) override;
    Executor*		reader(const IOObj&) override;

    EMBody*		getReadBody() override		{ return readbody_; }
    uiString		errMsg() const override		{ return errmsg_; };

protected:

    EMBody*		readbody_;
    uiString		errmsg_;
};


#define mDefineIndividualBodyTranslator(spec) \
mExpClass(EarthModel) spec##EMBodyTranslator : public odEMBodyTranslator \
{					       isTranslator(spec,EMBody) \
public: \
		spec##EMBodyTranslator( const char* nm,const char* unm ) \
		    : odEMBodyTranslator(nm,unm)	{} \
		~spec##EMBodyTranslator()		{} \
};

mDefineIndividualBodyTranslator(mc)
mDefineIndividualBodyTranslator(polygon)
mDefineIndividualBodyTranslator(randpos)



