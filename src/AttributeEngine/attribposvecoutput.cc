/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
________________________________________________________________________

-*/

#include "attribposvecoutput.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "seisioobjinfo.h"
#include "binidvalset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "ptrman.h"
#include "multiid.h"


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
    , aem_(0)
    , outex_(0)
    , msg_(tr("Done"))
{
    aem_ = new EngineMan;
    aem_->setAttribSet( &ads_ );
    aem_->setNLAModel( m );
    aem_->setUndefValue( mUdf(float) );
    outex_ = aem_->createFeatureOutput( inps_, bvss_ );
    if ( outex_ )
	setName( outex_->name() );
}


PosVecOutputGen::~PosVecOutputGen()
{
    delete aem_; delete outex_;
}


uiString PosVecOutputGen::uiMessage() const
{
    if ( outex_ )
	return outex_->uiMessage();

    return msg_;
}


uiString PosVecOutputGen::uiNrDoneText() const
{ return outex_ ? outex_->uiNrDoneText() : tr("Positions handled"); }


int PosVecOutputGen::nextStep()
{
    if ( !outex_ ) return -1;

    int res = outex_->doStep();
    if ( res < 0 ) return res;
    if ( res == 0 )
    {
	for ( int idx=0; idx<bvss_.size(); idx++ )
	{
	    PosVecDataSet* pvds = new PosVecDataSet;
	    for ( int refidx=0; refidx<inps_.size(); refidx++ )
	    {
		BufferString ref = inps_.get( refidx );
		BufferString nm = ref;
		if ( IOObj::isKey(ref) )
		    nm = IOM().nameOf( ref );
		pvds->add( new DataColDef(nm,ref) );
	    }

	    if ( inps_.size() != bvss_[idx]->nrVals()-1 )
		const_cast<BinIDValueSet*>(bvss_[idx])->setNrVals(
						inps_.size()+1 );

	    pvds->data() = *bvss_[idx];
	    vdss_ += pvds;
	}
    }

    return res;
}

};//namespace
