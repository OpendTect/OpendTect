#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "multiid.h"
#include "offsetazimuth.h"
#include "prestackprocessor.h"

namespace PreStack
{

/*!
\brief Lateral stack
*/

mExpClass(PreStackProcessing) LateralStack : public Processor
{ mODTextTranslationClass(LateralStack);
public:
			mDefaultFactoryInstanciationBase(
				"LateralStack", tr("Super Gather") );
    static Processor*	createInstance();

			LateralStack();
			~LateralStack();

    bool		reset(bool force=true) override;

    bool		wantsInput(const BinID&) const override;
    bool		setPattern(const BinID& stepout,bool cross);
			//If cross if false, it will be a rectangle
    bool		isCross() const { return iscross_; }
    const BinID&	getPatternStepout() const { return patternstepout_; }
    const BinID&	getInputStepout() const override
			{ return inputstepout_; }
    bool		setOutputInterest(const BinID& relbid,bool) override;

    bool		prepareWork() override;
    uiString		errMsg() const override		{ return errmsg_; }

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    bool		isInPattern(const BinID&) const;
    bool		processOutput( const OffsetAzimuth&,const BinID&);
    static const char*	sKeyStepout()		{ return "Stepout"; }
    static const char*	sKeyCross()		{ return "Is cross"; }

    uiString		errmsg_;
    bool		doWork(od_int64,od_int64,int) override;
    od_int64		nrIterations() const override
			{ return offsetazi_.size(); }

    BinID		inputstepout_;
    BinID		patternstepout_;
    bool		iscross_ = true;

    TypeSet<OffsetAzimuth>	offsetazi_;
};

} // namespace PreStack
