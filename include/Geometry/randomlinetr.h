#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
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
{
public:
			mDefEmptyTranslatorBaseConstructor(RandomLineSet)

    virtual const char* read(Geometry::RandomLineSet&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char* write(const Geometry::RandomLineSet&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Geometry::RandomLineSet&,const IOObj*,
				 BufferString&);
    static bool		store(const Geometry::RandomLineSet&,const IOObj*,
			      BufferString&);
};


mExpClass(Geometry) dgbRandomLineSetTranslator : public RandomLineSetTranslator
{				isTranslator(dgb,RandomLineSet)
public:

			mDefEmptyTranslatorConstructor(dgb,RandomLineSet)

    const char*		read(Geometry::RandomLineSet&,Conn&) override;
    const char*		write(const Geometry::RandomLineSet&,Conn&) override;
};


