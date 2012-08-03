#ifndef createattriblog_h
#define createattriblog_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2008
 RCS:           $Id: createattriblog.h,v 1.5 2012-08-03 13:00:46 cvskris Exp $
 _______________________________________________________________________

-*/

#include "wellattribmod.h"
#include "binidvalset.h"

namespace Attrib { class DescSet; class SelSpec; class EngineMan; }
namespace Well { class Data; class ExtractParams; }
class NLAModel;
class TaskRunner;

mClass(WellAttrib) AttribLogExtractor
{
public:
				AttribLogExtractor(const Well::Data& wd)
				    : wd_(&wd)
				    , bidset_(BinIDValueSet(2,true))  
				    {}

    const TypeSet<BinIDValueSet::Pos>& positions() const { return  positions_; }
    const TypeSet<float>& 	depths() const  	{ return depths_; }
    const BinIDValueSet& 	bidset() const		{ return bidset_; } 

    bool                        extractData(Attrib::EngineMan&,TaskRunner* t=0);
    bool                        fillPositions(const StepInterval<float>&);
    void			setWD(const Well::Data& wd)
				{ wd_ = &wd; }

protected:

    const Well::Data*		wd_;
    TypeSet<BinIDValueSet::Pos> positions_;
    BinIDValueSet 		bidset_; 
    TypeSet<float>		depths_;
};


mClass(WellAttrib) AttribLogCreator
{
public:

    mClass(WellAttrib) Setup
    {
    public:
				Setup(const Attrib::DescSet* attr,
					const Well::ExtractParams* wep)
				    : nlamodel_(0)
				    , attrib_(attr)
				    , tr_(0)
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

    bool 			doWork(Well::Data&,BufferString&);

protected:

    const Setup&		setup_;
    AttribLogExtractor*		extractor_;
    int&			sellogidx_;

    bool                        extractData(BinIDValueSet&);
    bool                        createLog(Well::Data&,const AttribLogExtractor&);

};



#endif

