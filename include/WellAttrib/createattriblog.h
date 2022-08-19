#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "binidvalset.h"
#include "bufstring.h"
#include "uistring.h"

namespace Attrib { class DescSet; class SelSpec; class EngineMan; }
namespace Well { class Data; class ExtractParams; }
class NLAModel;
class TaskRunner;

mExpClass(WellAttrib) AttribLogExtractor
{ mODTextTranslationClass(AttribLogExtractor);
public:
				AttribLogExtractor(const Well::Data& wd)
				    : wd_(&wd)
				    , bidset_(BinIDValueSet(2,true))
				    {}

    const TypeSet<BinIDValueSet::SPos>& positions() const { return positions_; }
    const TypeSet<float>&	depths() const	{ return depths_; }
    const BinIDValueSet&	bidset() const		{ return bidset_; }

    bool                        extractData(Attrib::EngineMan&,TaskRunner* t=0);
    bool                        fillPositions(const StepInterval<float>&);
    void			setWD(const Well::Data& wd)
				{ wd_ = &wd; }

protected:

    ConstRefMan<Well::Data>	wd_;
    TypeSet<BinIDValueSet::SPos> positions_;
    BinIDValueSet		bidset_;
    TypeSet<float>		depths_;
};


mExpClass(WellAttrib) AttribLogCreator
{ mODTextTranslationClass(AttribLogCreator);
public:

    mExpClass(WellAttrib) Setup
    {
    public:
				Setup(const Attrib::DescSet* attr,
					const Well::ExtractParams* wep)
				    : nlamodel_(nullptr)
				    , attrib_(attr)
				    , selspec_(nullptr)
				    , tr_(nullptr)
				    , extractparams_(wep)
				{}

	mDefSetupMemb(const NLAModel*,nlamodel)
	mDefSetupMemb(const Attrib::DescSet*,attrib)
	mDefSetupMemb(Attrib::SelSpec*,selspec)
	mDefSetupMemb(BufferString,lognm)
	mDefSetupMemb(const Well::ExtractParams*,extractparams)
	mDefSetupMemb(TaskRunner*,tr) //optional
    };


				AttribLogCreator(const Setup& su, int& selidx)
				    : setup_(su)
				    , extractor_(0)
				    , sellogidx_(selidx)
				    {}
				~AttribLogCreator() {}

    bool			doWork(Well::Data&,uiString&);

protected:

    const Setup&		setup_;
    AttribLogExtractor*		extractor_;
    int&			sellogidx_;

    bool                        extractData(BinIDValueSet&);
    bool			createLog(Well::Data&,
					  const AttribLogExtractor&);

};
