#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "transl.h"

class WellDataIOProvider;


/*!\brief Well TranslatorGroup */


mExpClass(Well) WellTranslatorGroup : public TranslatorGroup
{			    isTranslatorGroup(Well)
public:
			mDefEmptyTranslatorGroupConstructor(Well)
};


/*!  \brief Well Translator base class */

mExpClass(Well) WellTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(Well)

    mDeprecatedObs
    virtual const WellDataIOProvider&	getProv() const;

protected:

    bool		implRename(const IOObj*,const char*) const override;

};


/*!\brief WellTranslator for 'HDF5' stored wells, OD's newest well format. */

mExpClass(Well) hdfWellTranslator : public WellTranslator
{				isTranslator(hdf,Well)
public:
			mDefEmptyTranslatorConstructor(hdf,Well)

private:

    const char*		iconName() const override;
    const char*		defExtension() const override;
    bool		isUserSelectable(bool forread) const override;

    bool		implRename(const IOObj*,const char*) const override;

};


/*!\brief WellTranslator for 'dGB' stored wells, OD's legacy well format. */

mExpClass(Well) odWellTranslator : public WellTranslator
{				isTranslator(od,Well)
public:
			mDefEmptyTranslatorConstructor(od,Well)

private:

    const char*		iconName() const override;
    const char*		defExtension() const override;
    bool		isUserSelectable(bool forread) const override;

    bool		implRemove(const IOObj*,bool) const override;
    bool		implRename(const IOObj*,const char*) const override;
    bool		implSetReadOnly(const IOObj*,bool) const override;

};
