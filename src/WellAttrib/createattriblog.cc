/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: createattriblog.cc,v 1.2 2010-12-20 08:00:55 cvssatyaki Exp $";

#include "createattriblog.h"

#include "attribengman.h"
#include "attribprocessor.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"



#define mGetMrkDpth(mrk,nm,dpt)\
    mrk = nm ? wd.markers().getByName(nm) : 0;\
    if ( mrk ) dpt = mrk->dah(); 
void AttribLogCreator::setUpRange( const Well::Data& wd, Interval<float>& intv )
{
    const bool zinft = SI().zInFeet();

    float start = wd.track().dah(0);
    float stop = wd.track().dah( wd.track().size()-1);

    const Well::Marker* mrk = 0;
    mGetMrkDpth( mrk, setup_.topmrknm_, start )
    mGetMrkDpth( mrk, setup_.botmrknm_, stop )

    if ( start > stop )
    { float tmp; mSWAP( start, stop, tmp ); }
    intv.set( start, stop );
}


#define mErrRet(m) errmsg+=m; errmsg+="from well "; errmsg+=wd.name(); return false; 
bool AttribLogCreator::doWork( Well::Data& wd, BufferString& errmsg )
{
    Attrib::EngineMan aem;
    aem.setAttribSet( setup_.attrib_ );
    aem.setNLAModel( setup_.nlamodel_ );
    aem.setAttribSpec( *setup_.selspec_ );
    StepInterval<float> dahrg;
    setUpRange( wd, dahrg );

    AttribLogExtractor ale( wd );
    dahrg.step = setup_.extractstep_;
    if ( !ale.fillPositions(dahrg) )
    { mErrRet( "No positions extracted" ) }

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
	int posidx = mNINT(vidx);
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

