#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "executor.h"
#include "multiid.h"

namespace Well
{

mExpClass(Well) DatabaseUpdater : public Executor
{
mODTextTranslationClass(DatabaseUpdater)
public:
			DatabaseUpdater();
			~DatabaseUpdater();
			mOD_DisableCopy(DatabaseUpdater)

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    inline od_int64	nrDone() const override		{ return nrdone_; }
    inline od_int64	totalNr() const override	{ return totalnr_; }

protected:

private:
    int			nextStep() override;

    TypeSet<MultiID>	wellids_;
    od_int64		nrdone_				= 0;
    od_int64		totalnr_			= 0;
    uiStringSet		messages_;
};

} // namespace Well
