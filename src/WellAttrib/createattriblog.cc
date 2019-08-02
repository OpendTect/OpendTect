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



#define mErrRet(m) { errmsg.appendPhrase(m); return false; }


bool AttribLogCreator::doWork( Well::Data& wd, uiString& errmsg )
{
    if ( !setup_.extractparams_ )
	mErrRet(tr("Extraction parameters not set"))

    uiString msg = tr("%1 from well %2");
    Attrib::EngineMan aem;
    aem.setAttribSet( &setup_.attribdescset_ );
    aem.setNLAModel( setup_.nlamodel_ );
    aem.setAttribSpec( setup_.selspec_ );

    BufferStringSet dummy;
    StepInterval<float> dahrg = setup_.extractparams_->calcFrom( wd, dummy );
    if ( !mIsUdf( setup_.extractparams_->zstep_ ) )
	dahrg.step = setup_.extractparams_->zstep_;

    AttribLogExtractor ale( wd );
    if ( !ale.fillPositions(dahrg) )
    {
	msg.arg(tr("No positions extracted")).arg(wd.name());
	mErrRet(msg)
    }

    if ( !ale.extractData( aem, setup_.taskrunner_ ) )
    {
	msg.arg(tr("No data extracted")).arg(wd.name());
	mErrRet(msg)
    }

    if ( !createLog( wd, ale ) )
    {
	msg.arg(tr("Unable to create Log")).arg(wd.name());
	mErrRet(msg)
    }

    return true;
}


bool AttribLogCreator::createLog( Well::Data& wd, const AttribLogExtractor& ale)
{
    RefMan<Well::Log> newlog = new Well::Log( setup_.lognm_ );
    float v[2]; BinID bid;
    for ( int idx=0; idx<ale.depths().size(); idx++ )
    {
	ale.bidset().get( ale.positions()[idx], bid, v );
	if ( !mIsUdf(v[1]) )
	    newlog->addValue( ale.depths()[idx], v[1] );
    }

    if ( newlog->isEmpty() )
	return false;

    if ( sellogidx_ < 0 )
    {
	wd.logs().add( newlog );
	sellogidx_ = wd.logs().size() - 1;
    }
    else
    {
	RefMan<Well::Log> log = wd.logs().getLogByIdx( sellogidx_ );
	ChangeNotifyBlocker cnb( *log );
	log->setEmpty();
	Well::LogIter iter( *newlog );
	while ( iter.next() )
	    log->setValueAt( iter.dah(), iter.value() );
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
	const BinID bid = SI().transform( pos.getXY() );
	if ( !bid.inl() && !bid.crl() ) continue;

	if ( SI().zIsTime() )
	    pos.z_ = wd_->d2TModel().getTime( md, wd_->track() );
	bidset_.add( bid, (float) pos.z_, (float)idx );
	depths_ += md;
	positions_ += BinnedValueSet::SPos(0,0);
    }

    BinnedValueSet::SPos pos;
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
    uiRetVal uirv;
    ObjectSet<BinnedValueSet> bivsset;
    bivsset += &bidset_;
    PtrMan<Attrib::Processor> process =
	    aem.createLocationOutput( uirv, bivsset );
    if ( !process )
	return false;
    return TaskRunner::execute( taskr, *process );
}
