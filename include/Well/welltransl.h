#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "wellmod.h"
#include "transl.h"

class Executor;
class DataPointSet;
class BufferStringSet;
class WellDataIOProvider;

namespace Well { class Data; };

/*!\brief Well TranslatorGroup */


mExpClass(Well) WellTranslatorGroup : public TranslatorGroup
{			    isTranslatorGroup(Well)
public:
			mDefEmptyTranslatorGroupConstructor(Well)
    const char*		defExtension() const override { return "well"; }
};


/*!  \brief Well Translator base class */

mExpClass(Well) WellTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(Well)

    virtual const WellDataIOProvider&	getProv() const			= 0;

};


/*!\brief WellTranslator for 'dGB' stored wells, OD's default well format. */

mExpClass(Well) odWellTranslator : public WellTranslator
{				isTranslator(od,Well)
public:
				mDefEmptyTranslatorConstructor(od,Well)

    const WellDataIOProvider&	getProv() const override;

    bool		implRemove(const IOObj*,bool) const override;
    bool		implRename(const IOObj*,const char*) const override;
    bool		implSetReadOnly(const IOObj*,bool) const override;

};


