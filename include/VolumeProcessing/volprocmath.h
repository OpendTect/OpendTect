#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"

namespace Math { class Formula; }

namespace VolProc
{

mExpClass(VolumeProcessing) Math : public Step
{
mODTextTranslationClass(Math)
public:
			mDefaultFactoryInstantiation(
				Step, Math,
				"Math", tr("Mathematics") )
			~Math();

    void		setFormula(const char* expression);

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
			Math();

    bool		needsInput() const override		{ return true; }
    bool		isInputPrevStep() const override	{ return true; }
    bool		canInputAndOutputBeSame() const override{ return true; }
    bool		needsFullVolume() const override	{ return false;}
    bool		prefersBinIDWise() const override	{ return true; }
    bool		canHandle2D() const override		{ return true; }

    bool		prepareComp(int nrthreads) override;
    bool		computeBinID(const BinID&,int) override;

    ::Math::Formula*	formula_				= nullptr;
};

} // namespace VolProc
