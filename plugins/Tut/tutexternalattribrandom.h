#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"

#include "externalattrib.h"

namespace Stats { class RandomGenerator; }


namespace ExternalAttrib
{

//Class that calculates the random values

mExpClass(Tut) Random : public Attrib::ExtAttribCalc
{ mODTextTranslationClass(Random)
public:

    mDefaultFactoryInstantiation( ExtAttribCalc, Random,
				  "Tut_Random", tr("Random Attribute") );

    static BufferString		createDefinition();
    static uiString		createDisplayName();

    static bool			sCheckSelSpec(const Attrib::SelSpec&);

private:
				Random();
				~Random();

    bool		setTargetSelSpec(const Attrib::SelSpec&) override;

    ConstRefMan<RegularSeisDataPack> createAttrib(const TrcKeyZSampling&,
						  const RegularSeisDataPack*,
						  TaskRunner*) override;
    ConstRefMan<RegularSeisDataPack> createAttrib(const TrcKeyZSampling&,
						  TaskRunner*) override;
    bool		createAttrib(ObjectSet<BinIDValueSet>&,
				     TaskRunner*) override;
    bool		createAttrib(const BinIDValueSet&,SeisTrcBuf&,
				     TaskRunner*) override;
    ConstRefMan<RandomSeisDataPack> createRdmTrcAttrib(const ZGate&,
						const RandomLineID&,
						TaskRunner*) override;

    Stats::RandomGenerator&	gen_;
    BufferString		dpname_;
    const ZDomain::Info*	zdom_	= nullptr;
    ZSampling			zrg_	= ZSampling::udf();
};

} // namespace ExternalAttrib
