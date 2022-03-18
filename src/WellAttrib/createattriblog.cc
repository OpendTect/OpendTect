/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/

#include "createattriblog.h"

#include "attribengman.h"
#include "attribprocessor.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"



#define mErrRet(m) errmsg.append(m); return false;


bool AttribLogCreator::doWork( Well::Data& wdin, uiString& errmsg )
{
    RefMan<Well::Data> wd( &wdin );
    uiString msg = tr("%1 from well %2");
    Attrib::EngineMan aem;
    aem.setAttribSet( setup_.attrib_ );
    aem.setNLAModel( setup_.nlamodel_ );
    aem.setAttribSpec( *setup_.selspec_ );

    BufferStringSet dummy;
    StepInterval<float> dahrg = setup_.extractparams_->calcFrom( *wd, dummy );
    if ( !mIsUdf( setup_.extractparams_->zstep_ ) )
	dahrg.step = setup_.extractparams_->zstep_;

    AttribLogExtractor ale( *wd );
    if ( !ale.fillPositions(dahrg) )
    {
	msg.arg(tr("No positions extracted")).arg(wd->name());
	mErrRet(msg)
    }

    if ( !ale.extractData( aem, setup_.tr_ ) )
    {
	msg.arg(tr("No data extracted")).arg(wd->name());
	mErrRet(msg)
    }

    if ( !createLog( *wd, ale ) )
    {
	msg.arg(tr("Unable to create Log")).arg(wd->name());
	mErrRet(msg)
    }

    return true;
}


bool AttribLogCreator::createLog( Well::Data& wdin,
				  const AttribLogExtractor& ale)
{
    RefMan<Well::Data> wd( &wdin );
    Well::Log* newlog = new Well::Log( setup_.lognm_ );
    float v[2]; BinID bid;
    for ( int idx=0; idx<ale.depths().size(); idx++ )
    {
	ale.bidset().get( ale.positions()[idx], bid, v );
	if ( !mIsUdf(v[1]) )
	    newlog->addValue( ale.depths()[idx], v[1] );
    }

    if ( !newlog->size() )
    {
	delete newlog;
	return false;
    }

    if ( sellogidx_ < 0 )
    {
	wd->logs().add( newlog );
	sellogidx_ = wd->logs().size() - 1;
    }
    else
    {
	Well::Log& log = wd->logs().getLog( sellogidx_ );
	log.setEmpty();
	for ( int idx=0; idx<newlog->size(); idx++ )
	    log.addValue( newlog->dah(idx), newlog->value(idx) );
	delete newlog;
    }
    return true;
}




bool AttribLogExtractor::fillPositions(const StepInterval<float>& dahintv )
{
    bidset_.setEmpty(); positions_.erase(); depths_.erase();
    const int nrsteps = dahintv.nrSteps();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = dahintv.atIndex( idx );
	Coord3 pos = wd_->track().getPos( md );
	const BinID bid = SI().transform( pos );
	if ( !bid.inl() && !bid.crl() ) continue;

	if ( SI().zIsTime() && wd_->d2TModel() )
	    pos.z = wd_->d2TModel()->getTime( md, wd_->track() );
	bidset_.add( bid, (float) pos.z, (float)idx );
	depths_ += md;
	positions_ += BinIDValueSet::SPos(0,0);
    }

    BinIDValueSet::SPos pos;
    while ( bidset_.next(pos) )
    {
	float& vidx = bidset_.getVals(pos)[1];
	int posidx = mNINT32(vidx);
	positions_[posidx] = pos;
	mSetUdf(vidx);
    }
    return ( !positions_.isEmpty() && !depths_.isEmpty() && !bidset_.isEmpty());
}


bool AttribLogExtractor::extractData( Attrib::EngineMan& aem,
				      TaskRunner* taskr )
{
    uiString errmsg;
    ObjectSet<BinIDValueSet> bivsset;
    bivsset += &bidset_;
    PtrMan<Attrib::Processor> process =
	    aem.createLocationOutput( errmsg, bivsset );
    if ( !process )
	return false;
    return TaskRunner::execute( taskr, *process );
}
