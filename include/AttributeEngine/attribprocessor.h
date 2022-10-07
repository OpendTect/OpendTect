#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "executor.h"
#include "binid.h"

class TrcKeyZSampling;
class SeisTrcInfo;
namespace Seis { class SelData; }

namespace Attrib
{
class DataHolder;
class Desc;
class Output;
class Provider;

/*!
\brief Attribute Processor
*/

mExpClass(AttributeEngine) Processor : public Executor
{ mODTextTranslationClass(Processor)
public:
				Processor(Desc&,const char* linenm,
					  uiString& errmsg);
				~Processor();

    virtual bool		isOK() const;
    void			addOutput(Output*);
    void			setLineName(const char*);

    int				nextStep() override;
    void			init();
    od_int64			totalNr() const override;
    od_int64			nrDone() const override;
    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override
				{ return tr("Positions processed"); }

    void			addOutputInterest(int sel);
    bool			setZIntervals(
					TypeSet< Interval<int> >&,const BinID&,
					const Coord&,const TrcKey&);
    void			computeAndSetRefZStepAndZ0();

    Notifier<Attrib::Processor>	moveonly;
				/*!< triggered after a position is reached that
				     requires no processing, e.g. during initial
				     buffer fills. */

    const char*			getAttribName() const;
    const char*			getAttribUserRef() const;
    Provider*			getProvider()		{ return provider_; }
    ObjectSet<Output>	outputs_;

    void			setRdmPaths(const TypeSet<BinID>& truepath,
					    const TypeSet<BinID>& snappedpath);
				//for directional attributes

protected:
    void		useFullProcess(int&);
    void		useSCProcess(int&);
    void		fullProcess(const SeisTrcInfo*);

    void		defineGlobalOutputSpecs(TypeSet<int>&,TrcKeyZSampling&);
    void		prepareForTableOutput();
    void		computeAndSetPosAndDesVol(TrcKeyZSampling&);

    Desc&		desc_;
    Provider*		provider_;
    int			nriter_;
    int			nrdone_;
    bool		is2d_;
    TypeSet<int>	outpinterest_;
    uiString		errmsg_;
    bool		isinited_;
    bool		useshortcuts_;

    BinID		prevbid_;
    Seis::SelData*	sd_;

    bool		isHidingDataAvailabilityError() const;
    bool		showdataavailabilityerrors_;

public:
    void		showDataAvailabilityErrors(bool yn);
};


} // namespace Attrib
