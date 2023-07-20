/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratamp.h"

#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribparambase.h"
#include "attribprocessor.h"
#include "attribstorprovider.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "statruncalc.h"
#include "trckeyzsampling.h"


const char* StratAmpCalc::sKeySingleHorizonYN()	{ return "Is single horizon"; }
const char* StratAmpCalc::sKeyTopHorizonID()	{ return "Top horizon"; }
const char* StratAmpCalc::sKeyBottomHorizonID()	{ return "Bottom horizon"; }
const char* StratAmpCalc::sKeyAddToTopYN()	{ return "Add to top horizon"; }
const char* StratAmpCalc::sKeyAmplitudeOption()	{ return "Amplitude option"; }
const char* StratAmpCalc::sKeyOutputFoldYN()	{ return "Output fold"; }
const char* StratAmpCalc::sKeyTopShift()	{ return "Top shift"; }
const char* StratAmpCalc::sKeyBottomShift()	{ return "Bottom shift"; }
const char* StratAmpCalc::sKeyAttribName()	{ return "Attribute name"; }
const char* StratAmpCalc::sKeyIsOverwriteYN()	{ return "Overwrite"; }
const char* StratAmpCalc::sKeyIsClassification(){ return "Is Classification"; }

StratAmpCalc::StratAmpCalc( const EM::Horizon3D* tophor,
			    const EM::Horizon3D* bothor,
			    Stats::Type stattyp, const TrcKeySampling& hs,
			    bool outputfold )
    : Executor("Stratal amplitude Executor")
    , rdr_(0)
    , tophorizon_(tophor)
    , bothorizon_(bothor)
    , stattyp_(stattyp)
    , dataidx_(-1)
    , nrdone_(0)
    , usesstored_(false)
    , proc_(0)
    , hs_(hs)
    , outfold_(outputfold)
    , tophorshift_(mUdf(float))
    , bothorshift_(mUdf(float))
{
    TrcKeyZSampling cs;
    cs.hsamp_ = hs;
    totnr_ = hs.nrInl() * hs.nrCrl();

    if ( tophor ) tophor->ref();
    if ( bothor ) bothor->ref();

    descset_ = new Attrib::DescSet( false );

    msg_ = tr("Computing trace statistics");
}


StratAmpCalc::~StratAmpCalc()
{
    if ( tophorizon_ ) tophorizon_->unRef();
    if ( bothorizon_ ) bothorizon_->unRef();
    delete descset_;
    delete proc_;
    delete rdr_;
}


uiString StratAmpCalc::uiNrDoneText() const
{
    return ParallelTask::sTrcFinished();
}


int StratAmpCalc::init( const IOPar& pars )
{
    pars.get( sKeyTopShift(), tophorshift_ );
    pars.get( sKeyBottomShift(), bothorshift_ );
    if ( mIsUdf(tophorshift_) || mIsUdf(bothorshift_) )
	return -1;

    isclassification_ = false;
    pars.getYN( sKeyIsClassification(), isclassification_ );

    addtotop_ = false;
    pars.getYN( sKeyAddToTopYN(), addtotop_ );
    const EM::Horizon3D* addtohor = addtotop_ ? tophorizon_ : bothorizon_;
    if ( !addtohor )
	return -1;

    //determine whether stored data is used
    PtrMan<IOPar> attribs = pars.subselect("Attributes");
    if ( !attribs || !descset_->usePar(*attribs) )
	return -1;

    BufferString outpstr = IOPar::compKey( sKey::Output(), 0 );
    PtrMan<IOPar> outputpar = pars.subselect( outpstr );
    if ( !outputpar )
	return -1;

    BufferString attribidstr = IOPar::compKey( sKey::Attributes(), 0 );
    int attribid;
    if ( !outputpar->get(attribidstr,attribid) )
	return -1;

    Attrib::Desc* targetdesc =
			descset_->getDesc( Attrib::DescID(attribid,false) );
    if ( !targetdesc )
	return -1;

    BufferString defstring;
    targetdesc->getDefStr( defstring );
    BufferString storstr = Attrib::StorageProvider::attribName();
    usesstored_ = storstr.isStartOf( defstring );
    if ( usesstored_)
    {
	const LineKey lk( targetdesc->getValParam(
			Attrib::StorageProvider::keyStr())->getStringValue(0) );
	const MultiID key( lk.lineName().buf() );
	const BufferString attrnm = lk.attrName();
	PtrMan<IOObj> seisobj = IOM().get( key );
	if ( !seisobj )
	    return -1;

	TrcKeyZSampling cs;
	cs.hsamp_ = hs_;
	rdr_ = new SeisTrcReader( *seisobj, cs.hsamp_.getGeomID() );
	rdr_->setSelData( new Seis::RangeSelData(cs) );
	if ( !rdr_->prepareWork() )
	    return -1;
    }
    else
    {
	uiString errmsg;
	BufferString linename; //TODO: function used in 2d?
	PtrMan<Attrib::EngineMan> attrengman = new Attrib::EngineMan();
	proc_ = attrengman->usePar( pars, *descset_, linename, errmsg );
	if ( !proc_ )
	    return -1;
    }

    BufferString attribnm;
    pars.get( sKeyAttribName(), attribnm );
    if ( attribnm.isEmpty() )
	return -1;

    dataidx_ = addtohor->auxdata.auxDataIndex( attribnm );
    if ( dataidx_ < 0 )
	dataidx_ = addtohor->auxdata.addAuxData( attribnm );

    posid_.setObjectID( addtohor->id() );
    if ( outfold_ )
    {
	BufferString foldnm = attribnm;
	foldnm.add( "_fold" );
	dataidxfold_ = addtohor->auxdata.auxDataIndex( foldnm );
	if ( dataidxfold_ < 0 )
	    dataidxfold_ = addtohor->auxdata.addAuxData( foldnm );

	posidfold_.setObjectID( addtohor->id() );
    }

    return dataidx_;
}


#define mRet( event ) \
{ \
    delete trc; \
    if ( event == Executor::ErrorOccurred() ) \
	msg_ = rdr_->errMsg(); \
    return event; \
}

int StratAmpCalc::nextStep()
{
    if ( ( !proc_ && !rdr_ ) || !tophorizon_ || dataidx_<0 )
	return Executor::ErrorOccurred();

    int res = -1;
    SeisTrc* trc;
    if ( usesstored_ )
    {
	trc = new SeisTrc();
	const int rv = rdr_->get( trc->info() );
	if ( rv == 0 ) mRet( Executor::Finished() )
	else if ( rv == -1 ) mRet( Executor::ErrorOccurred() )
	if ( !rdr_->get(*trc) ) mRet( Executor::ErrorOccurred() );
    }
    else
    {
	res = proc_->nextStep();
	if ( res == 0 ) return Executor::Finished();
	if ( res == -1 )
	{
	    msg_ = proc_->uiMessage();
	    return Executor::ErrorOccurred();
	}

	trc = proc_->outputs_[0]->getTrc();
	if ( !trc )
	{
	    msg_ = proc_->uiMessage();
	    return Executor::ErrorOccurred();
	}
    }

    const BinID bid = trc->info().binID();
    const EM::SubID subid = bid.toInt64();
    float z1 = tophorizon_->getZ( bid );
    float z2 = !bothorizon_ ? z1 : bothorizon_->getZ( bid );
    if ( mIsUdf(z1) || mIsUdf(z2) )
	return Executor::MoreToDo();

    const StepInterval<float> zrg = trc->zRange();
    z1 += tophorshift_;
    const float snappedz1 = isclassification_
			  ? zrg.snap( z1, OD::SnapUpward ) : z1;

    z2 += bothorshift_;
    const float snappedz2 = isclassification_
			  ? zrg.snap( z2, OD::SnapDownward ) : z2;
    StepInterval<float> sampintv( snappedz1, snappedz2, zrg.step );
    sampintv.sort();
    sampintv.limitTo( zrg );

    Stats::CalcSetup rcsetup;
    rcsetup.require( stattyp_ );
    Stats::RunCalc<float> runcalc( rcsetup );
    for ( int idx=0; idx<sampintv.nrSteps()+1; idx++ )
    {
	const float zval = sampintv.atIndex( idx );
	const float val = trc->getValue( zval, 0 );
	if ( !mIsUdf(val) )
	    runcalc.addValue( val );
    }

    float outval = mUdf( float );
    switch ( stattyp_ )
    {
	case Stats::Min: outval = runcalc.min(); break;
	case Stats::Max: outval = runcalc.max(); break;
	case Stats::Average: outval = (float)runcalc.average(); break;
	case Stats::Median: outval = (float)runcalc.median(); break;
	case Stats::RMS: outval = (float)runcalc.rms(); break;
	case Stats::Sum: outval = runcalc.sum(); break;
	case Stats::MostFreq: outval = runcalc.mostFreq(); break;
	default: break;
    }

    const EM::Horizon3D* addtohor = addtotop_ ? tophorizon_ : bothorizon_;
    posid_.setSubID( subid );
    addtohor->auxdata.setAuxDataVal( dataidx_, posid_, (float)outval );
    if ( outfold_ )
    {
	posidfold_.setSubID( subid );
	addtohor->auxdata.setAuxDataVal( dataidxfold_, posidfold_,
					  mCast(float,runcalc.count()) );
    }

    nrdone_++;

    if ( usesstored_ )
	delete trc;
    else
	proc_->outputs_[0]->deleteTrc();

    return res || rdr_ ? Executor::MoreToDo() : Executor::Finished();
}


bool StratAmpCalc::saveAttribute( const EM::Horizon3D* hor, int attribidx,
				  bool overwrite, od_ostream* strm )
{
    PtrMan<Executor> datasaver =
			hor->auxdata.auxDataSaver( attribidx, overwrite );
    if ( !(datasaver && datasaver->go(strm,false,false)) )
	return false;

    if ( outfold_ )
    {
	datasaver.erase();
	datasaver = hor->auxdata.auxDataSaver( dataidxfold_, overwrite );
	if ( !(datasaver && datasaver->go(strm,false,false)) )
	    return false;
    }

    return true;
}
