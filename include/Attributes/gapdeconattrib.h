#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"

namespace Attrib
{

/*!
\brief Gap deconvolution attribute.
*/

mClass(Attributes) GapDecon : public Provider
{
public:

    static void			initClass();
				GapDecon(Desc&);

    static const char*		attribName()	{ return "GapDecon"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		lagsizeStr()	{ return "lagsize"; }
    static const char*		gapsizeStr()	{ return "gapsize"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		noiselevelStr()	{ return "noiselevel"; }
    static const char*		isinp0phaseStr(){ return "isinpzerophase"; }
    static const char*		isout0phaseStr(){ return "isoutzerophase"; }
    static const char*		onlyacorrStr()	{ return "onlyautocorr"; }

    void			prepareForComputeData() override;

protected:

    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const override
				{ return false; }

    bool			getInputData(const BinID&,int zintv) override;
    bool			computeData(const DataHolder&,
					    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const override;
    const Interval<int>*	desZSampMargin(int,int) const override
				{ return &dessampgate_; }

    Interval<float>		gate_;
    int				lagsize_;
    int				gapsize_;
    int				noiselevel_;

    bool			isinpzerophase_;
    bool			isoutzerophase_;
    BinID			stepout_;
    bool			useonlyacorr_;

    int				nlag_;
    int				ncorr_;
    int				lcorr_;
    int				ngap_;

    const DataHolder*		inputdata_;
    const DataHolder*		inputdatamixed_;
    int				dataidx_;
    int				dataidxmixed_;

    const float*		hilbfilter_;
    Interval<int>		dessampgate_;
};

} // namespace Attrib
