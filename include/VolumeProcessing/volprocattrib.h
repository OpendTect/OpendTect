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
#include "dbkey.h"
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

    static Attrib::Provider* createInstance(Attrib::Desc&);

    virtual bool	allowParallelComputation() const;
    virtual bool	computeData(const Attrib::DataHolder&,
				    const BinID&,int z0,
				    int nrsamples,int threadid) const;

    virtual void	prepareForComputeData();

    VolProc::Chain*	chain_;
    DBKey		setupmid_;
    VolProc::ChainExecutor* executor_;
};



/*!\brief Adapter for a VolProc chain to external attribute calculation. */

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

    static BufferString	createDefinition(const DBKey& setup);

    bool		setTargetSelSpec(const Attrib::SelSpec&);

    virtual RefMan<RegularSeisDataPack>
			createAttrib(const TrcKeyZSampling&,DataPack::ID,
				     TaskRunner*);
    virtual bool	createAttrib( ObjectSet<BinnedValueSet>& o,
				      TaskRunner* tskr )
			{ return Attrib::ExtAttribCalc::createAttrib(o,tskr); }
    virtual bool	createAttrib( const BinnedValueSet& b, SeisTrcBuf& tb,
				      TaskRunner* tskr )
			{
			return Attrib::ExtAttribCalc::createAttrib(b,tb,tskr);
			}
protected:

    static Attrib::ExtAttribCalc* create(const Attrib::SelSpec&);

    Chain*			chain_;
    DBKey			rendermid_;

};

} // namespace VolProc
