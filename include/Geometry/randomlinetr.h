#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

 
#include "geometrymod.h"
#include "transl.h"
#include "bufstringset.h"

namespace Geometry { class RandomLineSet; }
class Conn;

mExpClass(Geometry) RandomLineSetTranslatorGroup : public TranslatorGroup
{				  isTranslatorGroup(RandomLineSet)
public:
			mDefEmptyTranslatorGroupConstructor(RandomLineSet)

    const char*		defExtension() const override	{ return "rdl"; }
};


mExpClass(Geometry) RandomLineSetTranslator : public Translator
{ mODTextTranslationClass(RandomLineSetTranslator)
public:
			mDefEmptyTranslatorBaseConstructor(RandomLineSet)

    virtual uiString	 read(Geometry::RandomLineSet&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual uiString	 write(const Geometry::RandomLineSet&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Geometry::RandomLineSet&,const IOObj*,
								    uiString&);
    static bool		store(const Geometry::RandomLineSet&,const IOObj*,
								    uiString&);
};


mExpClass(Geometry) dgbRandomLineSetTranslator : public RandomLineSetTranslator
{ mODTextTranslationClass(dgbRandomLineSetTranslator)
					isTranslator(dgb,RandomLineSet)
public:

			mDefEmptyTranslatorConstructor(dgb,RandomLineSet)

    uiString		read(Geometry::RandomLineSet&,Conn&) override;
    uiString		write(const Geometry::RandomLineSet&,Conn&) override;
};
