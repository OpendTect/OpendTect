/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: attribposvecoutput.cc,v 1.1 2005-07-28 10:53:50 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribposvecoutput.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "seisioobjinfo.h"
#include "binidvalset.h"
#include "posvecdataset.h"
#include "featset.h"
#include "ptrman.h"
#include "multiid.h"

#define mErrRet(s) { failed_ = true; msg_ = s; }

namespace Attrib
{
    
PosVecOutputGen::PosVecOutputGen( const DescSet& as,
				  const BufferStringSet& in,
				  const ObjectSet<BinIDValueSet>& b,
				  ObjectSet<PosVecDataSet>& v,
				  const NLAModel* m )
	: Executor("Extracting")
	, ads_(as)
	, inps_(in)
	, bvss_(b)
	, vdss_(v)
	, nlamodel_(m)
	, curlnr_(0)
	, aem_(0)
	, outex_(0)
	, msg_("Scanning data")
	, failed_(false)
{
    if ( !ads_.is2D() )
	linenames_.add( "" );
    else
    {
	PtrMan<BinIDValueSet> allbivs = 0;
	for ( int idx=0; idx<bvss_.size(); idx++ )
	{
	    if ( !idx )
		allbivs = new BinIDValueSet( *bvss_[idx] );
	    else
		allbivs->append( *bvss_[idx] );
	}
	if ( allbivs->isEmpty() )
	    mErrRet("No data extraction points")

	MultiID key;
	if ( !ads_.getFirstStored(Only2D,key) )
	    mErrRet("Cannot find line set in attribute set")

	SeisIOObjInfo oi( key );
	if ( !oi.isOK() )
	    mErrRet("Cannot find line set used in attribute set")
	oi.getLineNames( linenames_, false, allbivs );
	if ( linenames_.size() < 1 )
	    mErrRet("No line with any extraction position found")
    }

    nextExec();
}


void PosVecOutputGen::cleanUp()
{
    delete aem_; delete outex_; deepErase( workfss_ );
}

const char* PosVecOutputGen::message() const
{
    if ( !outex_ ) return msg_.buf();

    msg_ = outex_->message();
    if ( curlnr_ < linenames_.size() )
    {
	const char* lnm = linenames_.get(curlnr_);
	if ( lnm && *lnm )
	{
	    msg_ += " for ";
	    msg_ += linenames_.get(curlnr_);
	}
    }
    return msg_.buf();
}


void PosVecOutputGen::nextExec()
{
    cleanUp();
    aem_ = new EngineMan;
    aem_->setAttribSet( &ads_ );
    aem_->setNLAModel( nlamodel_ );
    aem_->setUndefValue( mUndefValue );
    aem_->setLineKey( linenames_.get(curlnr_) );
    outex_ = aem_->featureOutputCreator( inps_, bvss_, workfss_ );
    setName( outex_->name() );
}


void PosVecOutputGen::addResults()
{
    if ( curlnr_ == 0 )
    {
	// The problem is that some feature sets may be empty
	// In that case the descs are also bad
	// So we need to find a template featset which is filled
	FeatureSet* tmplfs = 0;
	for ( int idx=0; idx<workfss_.size(); idx++ )
	{
	    vdss_ += new PosVecDataSet;
	    FeatureSet* fs = workfss_[idx];
	    if ( !tmplfs && fs->descs().size() > 0 && fs->size() > 0 )
	    {
		tmplfs = fs;
		fs->fill( *vdss_[idx] );
		fs->erase();
	    }
	}
	if ( !tmplfs )
	    return;

	for ( int idx=0; idx<workfss_.size(); idx++ )
	{
	    FeatureSet* fs = workfss_[idx];
	    PosVecDataSet* vds = vdss_[idx];
	    if ( fs == tmplfs )
		continue;
	    else if ( fs->size() == 0 )
		tmplfs->fill( *vds );
	    else
		fs->fill( *vds );
	}
	deepErase( workfss_ );
    }
    else
    {
	for ( int ifs=0; ifs<workfss_.size(); ifs++ )
	{
	    FeatureSet& genfs = *workfss_[ifs];
	    const int nrcols = genfs.descs().size() + 1;
	    float vals[nrcols];
	    PosVecDataSet& vds = *vdss_[ifs];
	    while ( genfs.size() )
	    {
		FeatureVec* fv = genfs.releaseLast();
		vals[0] = fv->fvPos().ver;
		for ( int iv=0; iv<fv->size(); iv++ )
		    vals[iv+1] = (*fv)[iv];
		vds.data().add( fv->fvPos(), vals );
		delete fv;
	    }
	}
	deepErase( workfss_ );
    }
}


int PosVecOutputGen::nextStep()
{
    if ( failed_ || !outex_ ) return -1;

    int res = outex_->doStep();
    if ( res < 0 ) return res;
    if ( res == 0 )
    {
	addResults();

	curlnr_++;
	if ( curlnr_ >= linenames_.size() )
	    return 0;
	nextExec();
	return 1;
    }

    return 1;
}

};//namespace
