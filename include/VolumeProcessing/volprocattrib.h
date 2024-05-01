#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"

#include "attribparam.h"
#include "attribprovider.h"
#include "externalattrib.h"
#include "multiid.h"
#include "uistring.h"
#include "volprocchain.h"

namespace VolProc { class ChainExecutor; }

mExpClass(VolumeProcessing) VolProcAttrib : public Attrib::Provider
{ mODTextTranslationClass(VolProcAttrib);
public:
    static void		initClass();

    static const char*	attribName()		{ return "VolumeProcessing"; }
    static const char*	sKeySetup()		{ return "setup"; }

protected:
			VolProcAttrib(Attrib::Desc&);
			~VolProcAttrib();

    static Attrib::Provider*	createInstance(Attrib::Desc&);

    bool		allowParallelComputation() const override;
    bool		computeData(const Attrib::DataHolder&,
				    const BinID&,int z0,
				    int nrsamples,int threadid) const override;

    void		prepareForComputeData() override;

    RefMan<VolProc::Chain> chain_;
    MultiID		setupmid_;
    VolProc::ChainExecutor* executor_ = nullptr;
};



/*!
\brief Adapter for a VolProc chain to external attribute calculation.
*/

namespace VolProc
{

mExpClass(VolumeProcessing) ExternalAttribCalculator
					: public Attrib::ExtAttribCalc
{ mODTextTranslationClass(ExternalAttribCalculator);
public:

    static BufferString createDefinition(const MultiID& setup);

    static const char*	sAttribName()	{ return "Volume_Processing"; }
    static const char*	sKeySetup()	{ return "volprocsetup"; }

private:
			ExternalAttribCalculator();
			~ExternalAttribCalculator();

    bool		setTargetSelSpec(const Attrib::SelSpec&) override;

    ConstRefMan<RegularSeisDataPack> createAttrib(const TrcKeyZSampling&,
					const RegularSeisDataPack*,
					TaskRunner*) override;
    ConstRefMan<RegularSeisDataPack> createAttrib(const TrcKeyZSampling&,
						  TaskRunner*) override;
    bool		createAttrib( ObjectSet<BinIDValueSet>& o,
				      TaskRunner* trans ) override
			{ return Attrib::ExtAttribCalc::createAttrib(o,trans); }
    bool		createAttrib( const BinIDValueSet& b, SeisTrcBuf& tb,
				      TaskRunner* trans ) override
			{ return Attrib::ExtAttribCalc::createAttrib(b,
								tb,trans); }

    static Attrib::ExtAttribCalc* create(const Attrib::SelSpec&);

    RefMan<Chain>		chain_;
    MultiID			rendermid_;

public:

    static void		initClass();

};

} // namespace VolProc
