#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "mathfunc.h"

namespace Stats { class RandomGenerator; }

/*!%CEEMD Attributes

Input:
0		Data

Outputs:
0		CEEMD attributes
*/

namespace Attrib
{

class CEEMD: public Provider
{
public:
    static void		initClass();
			CEEMD(Desc&);

    static const char*	attribName()		{ return "CEEMD"; }
    static const char*	stopimfStr()		{ return "stopimf"; }
    static const char*	stopsiftStr()		{ return "stopsift"; }
    static const char*	maxnrimfStr()		{ return "maxnrimf"; }
    static const char*	maxsiftStr()		{ return "maxsift"; }
    static const char*	emdmethodStr()		{ return "method"; }
    static const char*	attriboutputStr()	{ return "attriboutput"; }
    static const char*	symmetricboundaryStr()	{ return "symmetricboundary"; }
    static const char*	noisepercentageStr()	{ return "noisepercentage"; }
    static const char*	maxnoiseloopStr()	{ return "maxnoiseloop"; }
    static const char*	minstackcountStr()	{ return "minstackcount"; }
    static const char*	outputfreqStr()		{ return "outputfreq"; }
    static const char*	stepoutfreqStr()	{ return "stepoutfreq"; }
    static const char*	outputcompStr()		{ return "outputcomp"; }
    static const char*	usetfpanelStr()		{ return "usetfpanel"; }
    static const char*	transMethodNamesStr(int);
    static const char*	transOutputNamesStr(int);
    void		getCompNames(BufferStringSet& nms) const override;
    bool		prepPriorToOutputSetup() override;

protected:
			~CEEMD();

    static Provider*	createInstance(Desc&);
    static void		updateDefaults(Desc&)	{};
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const override
			{ return false; }
    bool		areAllOutputsEnabled() const;
    void		getFirstAndLastOutEnabled(int& first,int& last) const;
    int			maxnrimf_; // Maximum number of intrinsic Mode Functions
    int			maxsift_; // Maximum number of sifting iterations
    float		outputfreq_; // Output frequency
    float		stepoutfreq_; // Step output frequency
    int			outputcomp_; // Output frequency
    float		stopsift_; // stop sifting if st.dev res.-imf < value
    float		stopimf_; // stop decomp. when st.dev imf < value
    float		noisepercentage_; // noise percentage for EEMD and CEEMD
			// boundary extension symmetric or periodic
    bool		symmetricboundary_;
			// use synthetic trace in ceemdtestprogram.h
    bool		usetestdata_;
    bool		usetfpanel_; // if panel is pressed use 0 to Nyquist
    bool		getInputOutput(int input,TypeSet<int>&) const override;
    bool		getInputData(const BinID&,int zintv) override;
    bool		computeData(const DataHolder&,const BinID& relpos,
			    int z0,int nrsamples,int threadid) const override;
    const Interval<float>* reqZMargin(int input,int output) const override
			   { return &gate_; }
    const Interval<int>* desZSampMargin(int input,int output) const override
			 { return &dessampgate_; }

    Interval<float>	gate_;
    Interval<int>	dessampgate_;
    int			dataidx_;
    int			method_;
    int			attriboutput_;
    int			maxnoiseloop_;
    const DataHolder*	inputdata_ = nullptr;
    Stats::RandomGenerator* gen_ = nullptr;
};

} // namespace Attrib
