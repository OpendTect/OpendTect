#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"

#include "attribparam.h"
#include "attribprovider.h"
#include "externalattrib.h"
#include "multiid.h"
#include "uistring.h"

namespace VolProc { class Chain; class ChainExecutor; }

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

    VolProc::Chain*	chain_;
    MultiID		setupmid_;
    VolProc::ChainExecutor* executor_;
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
    static void		initClass();
			ExternalAttribCalculator();
			~ExternalAttribCalculator();

    static const char*	sAttribName()	{ return "Volume_Processing"; }
    static const char*	sKeySetup()	{ return "volprocsetup"; }

    static BufferString	createDefinition(const MultiID& setup);

    bool		setTargetSelSpec(const Attrib::SelSpec&) override;

    DataPack::ID	createAttrib(const TrcKeyZSampling&,DataPack::ID,
				     TaskRunner*) override;
    bool		createAttrib( ObjectSet<BinIDValueSet>& o,
				      TaskRunner* trans ) override
			{ return Attrib::ExtAttribCalc::createAttrib(o,trans); }
    bool		createAttrib( const BinIDValueSet& b, SeisTrcBuf& tb,
				      TaskRunner* trans ) override
			{ return Attrib::ExtAttribCalc::createAttrib(b,
								tb,trans); }
    DataPack::ID	createAttrib(const TrcKeyZSampling&,
				      const LineKey&,TaskRunner*) override;

protected:

    static Attrib::ExtAttribCalc* create(const Attrib::SelSpec&);

    Chain*			chain_;
    MultiID			rendermid_;

};

} // namespace VolProc

