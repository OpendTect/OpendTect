/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: createattriblog.cc,v 1.6 2012/07/10 13:06:04 cvskris Exp $";

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



#define mErrRet(m) errmsg+=m; errmsg+="from well "; errmsg+=wd.name(); return false; 
bool AttribLogCreator::doWork( Well::Data& wd, BufferString& errmsg )
{
    Attrib::EngineMan aem;
    aem.setAttribSet( setup_.attrib_ );
    aem.setNLAModel( setup_.nlamodel_ );
    aem.setAttribSpec( *setup_.selspec_ );

    BufferStringSet dummy;
    StepInterval<float> dahrg = setup_.extractparams_->calcFrom( wd, dummy );
    if ( !mIsUdf( setup_.extractparams_->zstep_ ) )
	dahrg.step = setup_.extractparams_->zstep_;

    AttribLogExtractor ale( wd );
    if ( !ale.fillPositions(dahrg) )
    { mErrRet( "No positions extracted " ) }

    if ( !ale.extractData( aem, setup_.tr_ ) )
    { mErrRet( "No data extracted" ) }

    if ( !createLog( wd, ale ) )
    { mErrRet( "Unable to create Log" ) }

    return true;
}


bool AttribLogCreator::createLog( Well::Data& wd, const AttribLogExtractor& ale)
{
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
	wd.logs().add( newlog );
	sellogidx_ = wd.logs().size() - 1;
    }
    else
    {
	Well::Log& log = wd.logs().getLog( sellogidx_ );
	log.erase();
	for ( int idx=0; idx<newlog->size(); idx++ )
	    log.addValue( newlog->dah(idx), newlog->value(idx) );
	delete newlog;
    }
    return true;
}




bool AttribLogExtractor::fillPositions(const StepInterval<float>& dahintv )
{
    bidset_.empty(); positions_.erase(); depths_.erase();
    const int nrsteps = dahintv.nrSteps();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = dahintv.atIndex( idx );
	Coord3 pos = wd_->track().getPos( md );
	const BinID bid = SI().transform( pos );
	if ( !bid.inl && !bid.crl ) continue;

	if ( SI().zIsTime() && wd_->d2TModel() )
	    pos.z = wd_->d2TModel()->getTime( md );
	bidset_.add( bid, pos.z, (float)idx );
	depths_ += md;
	positions_ += BinIDValueSet::Pos(0,0);
    }
    
    BinIDValueSet::Pos pos;
    while ( bidset_.next(pos) )
    {
	float& vidx = bidset_.getVals(pos)[1];
	int posidx = mNINT32(vidx);
	positions_[posidx] = pos;
	mSetUdf(vidx);
    }
    return ( !positions_.isEmpty() && !depths_.isEmpty() && !bidset_.isEmpty());
}


bool AttribLogExtractor::extractData( Attrib::EngineMan& aem, TaskRunner* tr )
{
    BufferString errmsg;
    ObjectSet<BinIDValueSet> bivsset;
    bivsset += &bidset_;
    PtrMan<Attrib::Processor> process =
	    aem.createLocationOutput( errmsg, bivsset );
    if ( !process ) 
	return false;
    return tr ? tr->execute( *process ) : process->execute();
}

