#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "transl.h"

class Executor;
class DataPointSet;
class BufferStringSet;
class WellDataIOProvider;

namespace Well { class Data; }

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

protected:

    bool		implRename_(const IOObj*,const char*) const;

};


/*!\brief WellTranslator for 'dGB' stored wells, OD's legacy well format. */

mExpClass(Well) odWellTranslator : public WellTranslator
{				isTranslator(od,Well)
public:
			mDefEmptyTranslatorConstructor(od,Well)

    const WellDataIOProvider&	getProv() const override;

    bool		implRemove(const IOObj*,bool) const override;
    bool		implRename(const IOObj*,const char*) const override;
    bool		implSetReadOnly(const IOObj*,bool) const override;

};

mGlobal(Well) void inithdfWellTranslator();
