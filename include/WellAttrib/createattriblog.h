#pragma once
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2008
 _______________________________________________________________________

-*/

#include "wellattribmod.h"
#include "attribsel.h"
#include "binnedvalueset.h"
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
				    , bidset_(BinnedValueSet(2,true))
				    {}

    const TypeSet<BinnedValueSet::SPos>& positions() const { return positions_; }
    const TypeSet<float>&	depths() const	{ return depths_; }
    const BinnedValueSet&	bidset() const		{ return bidset_; }

    bool                        extractData(Attrib::EngineMan&,TaskRunner* t=0);
    bool                        fillPositions(const StepInterval<float>&);
    void			setWD(const Well::Data& wd)
				{ wd_ = &wd; }

protected:

    const Well::Data*		wd_;
    TypeSet<BinnedValueSet::SPos> positions_;
    BinnedValueSet		bidset_;
    TypeSet<float>		depths_;
};


mExpClass(WellAttrib) AttribLogCreator
{ mODTextTranslationClass(AttribLogCreator);
public:

    mExpClass(WellAttrib) Setup
    {
    public:
				Setup( const Attrib::DescSet& ads,
					const Well::ExtractParams* wep )
				    : attribdescset_(ads)
				    , extractparams_(wep)
				    , nlamodel_(nullptr)
				    , selspec_(nullptr)
				    , taskrunner_(nullptr)
				    {}

	mDefSetupMemb(const NLAModel*,nlamodel)
	mDefSetupMemb(Attrib::SelSpec,selspec)
	mDefSetupMemb(BufferString,lognm)
	mDefSetupMemb(TaskRunner*,taskrunner)

	const Attrib::DescSet&	    attribdescset_;
	const Well::ExtractParams*  extractparams_;
    };


				AttribLogCreator( const Setup& su, int selidx )
				    : setup_(su)
				    , sellogidx_(selidx)
				    , extractor_(0)		{}
				~AttribLogCreator()		{}

    bool			doWork(Well::Data&,uiString&);

protected:

    const Setup			setup_;
    int				sellogidx_;
    AttribLogExtractor*		extractor_;

    bool                        extractData(BinnedValueSet&);
    bool                        createLog(Well::Data&,
					  const AttribLogExtractor&);

};
