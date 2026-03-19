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
#include "hiddenparam.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "statruncalc.h"
#include "trckeyzsampling.h"


const char* StratAmpCalc::sKeySingleHorizonYN()
	    { return "Is single horizon"; }
const char* StratAmpCalc::sKeyTopHorizonID()
	    { return "Top horizon"; }
const char* StratAmpCalc::sKeyBottomHorizonID()
	    { return "Bottom horizon"; }
const char* StratAmpCalc::sKeyAddToTopYN()	{ return "Add to top horizon"; }
const char* StratAmpCalc::sKeyAmplitudeOption()
	    { return "Amplitude option"; }
const char* StratAmpCalc::sKeyOutputFoldYN()	{ return "Output fold"; }
const char* StratAmpCalc::sKeyTopShift()	{ return "Top shift"; }
const char* StratAmpCalc::sKeyBottomShift()	{ return "Bottom shift"; }
const char* StratAmpCalc::sKeyAttribName()	{ return "Attribute name"; }
const char* StratAmpCalc::sKeyIsOverwriteYN()	{ return "Overwrite"; }
const char* StratAmpCalc::sKeyIsClassification()
	    { return "Is Classification"; }

static HiddenParam<StratAmpCalc,TypeSet<int>*> hp_selcomps( nullptr );
static HiddenParam<StratAmpCalc,TypeSet<int>*> hp_dataidxs( nullptr );
static HiddenParam<StratAmpCalc,TypeSet<int>*> hp_dataidxsfold( nullptr );

StratAmpCalc::StratAmpCalc( const EM::Horizon3D* tophor,
			    const EM::Horizon3D* bothor,
			    Stats::Type stattyp, const TrcKeySampling& hs,
			    bool outputfold )
    : Executor("Stratal amplitude Executor")
    , stattyp_(stattyp)
    , rdr_(nullptr)
    , usesstored_(false)
    , tophorizon_(tophor)
    , bothorizon_(bothor)
    , nrdone_(0)
    , tophorshift_(mUdf(float))
    , bothorshift_(mUdf(float))
    , dataidx_(-1)
    , outfold_(outputfold)
    , hs_(hs)
    , proc_(nullptr)
{
    hp_selcomps.setParam( this, new TypeSet<int> );
    hp_dataidxs.setParam( this, new TypeSet<int> );
    hp_dataidxsfold.setParam( this, new TypeSet<int> );
    TrcKeyZSampling cs;
    cs.hsamp_ = hs;
    totnr_ = hs.nrInl() * hs.nrCrl();
    descset_ = new Attrib::DescSet( false );
    msg_ = tr("Computing trace statistics");
}


StratAmpCalc::~StratAmpCalc()
{
    hp_selcomps.removeAndDeleteParam( this );
    hp_dataidxs.removeAndDeleteParam( this );
    hp_dataidxsfold.removeAndDeleteParam( this );
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
    return doInit( pars ) ? 0 : -1;
}


bool StratAmpCalc::doInit( const IOPar& pars )
{
    auto* selcomps = hp_selcomps.getParam( this );
    auto* dataidxs = hp_dataidxs.getParam( this );
    auto* dataidxsfold = hp_dataidxsfold.getParam( this );
    if ( !tophorizon_ )
	return false;

    if ( !addtotop_ && !bothorizon_ )
	return false;

    pars.get( sKeyTopShift(), tophorshift_ );
    pars.get( sKeyBottomShift(), bothorshift_ );
    if ( mIsUdf(tophorshift_) || mIsUdf(bothorshift_) )
	return false;

    isclassification_ = false;
    pars.getYN( sKeyIsClassification(), isclassification_ );

    addtotop_ = false;
    pars.getYN( sKeyAddToTopYN(), addtotop_ );
    const EM::Horizon3D& addtohor = addtotop_ ? *tophorizon_ : *bothorizon_;

    //determine whether stored data is used
    PtrMan<IOPar> attribs = pars.subselect("Attributes");
    if ( !attribs || !descset_->usePar(*attribs) )
	return false;

    const BufferString outpstr = IOPar::compKey( sKey::Output(), 0 );
    ConstPtrMan<IOPar> outputpar = pars.subselect( outpstr );
    if ( !outputpar )
	return false;

    const BufferString nroutputkey = IOPar::compKey( sKey::Attributes(),
					    Attrib::DescSet::highestIDStr() );
    int nroutputs = 1;
    outputpar->get( nroutputkey.buf(), nroutputs );

    for ( int idx=0; idx<nroutputs; idx++ )
    {
	BufferString attribidstr = IOPar::compKey( sKey::Attributes(), idx );
	int attribid;
	if ( !outputpar->get(attribidstr,attribid) )
	    return false;

	RefMan<Attrib::Desc> targetdesc =
			    descset_->getDesc( Attrib::DescID(attribid,false) );
	if ( !targetdesc )
	    return false;

	RefMan<Attrib::Desc> inpdesc = targetdesc->getInput(0);
	const int selcomp = inpdesc ? inpdesc->selectedOutput()
				    : targetdesc->selectedOutput();
	if ( selcomp >= 0 )
	    selcomps->add( selcomp );

	BufferString defstring;
	targetdesc->getDefStr( defstring );
	BufferString storstr = Attrib::StorageProvider::attribName();
	usesstored_ = storstr.isStartOf( defstring );
	if ( usesstored_)
	{
	    const MultiID key = targetdesc->getStoredID();
	    ConstPtrMan<IOObj> seisobj = IOM().get( key );
	    if ( !seisobj )
		return false;

	    TrcKeyZSampling cs;
	    cs.hsamp_ = hs_;
	    rdr_ = new SeisTrcReader( *seisobj, cs.hsamp_.getGeomID() );
	    rdr_->setSelData( new Seis::RangeSelData(cs) );
	    if ( !rdr_->prepareWork() )
		return false;
	}
	else if ( !proc_ )
	{
	    uiString errmsg;
	    BufferString linename; //TODO: function used in 2d?
	    PtrMan<Attrib::EngineMan> attrengman = new Attrib::EngineMan();
	    proc_ = attrengman->usePar( pars, *descset_, linename, errmsg );
	    if ( !proc_ )
		return false;
	}

	BufferString attribnm;
	pars.get( sKeyAttribName(), attribnm );
	if ( nroutputs>1 )
	    attribnm.addSpace().add( targetdesc->userRef() );

	if ( attribnm.isEmpty() )
	    return false;

	int dataidx = addtohor.auxdata.auxDataIndex( attribnm );
	if ( dataidx < 0 )
	    dataidx = addtohor.auxdata.addAuxData( attribnm );

	if ( dataidx >= 0 )
	    dataidxs->add( dataidx );

	posid_.setObjectID( addtohor.id() );
	if ( outfold_ )
	{
	    BufferString foldnm = attribnm;
	    foldnm.add( "_fold" );
	    int dataidxfold = addtohor.auxdata.auxDataIndex( foldnm );
	    if ( dataidxfold < 0 )
		dataidxfold = addtohor.auxdata.addAuxData( foldnm );

	    if ( dataidxfold >= 0 )
		dataidxsfold->add( dataidxfold );

	    posidfold_.setObjectID( addtohor.id() );
	}
    }

    if ( dataidxs->size() != selcomps->size() )
	return false;

    if ( outfold_ && dataidxs->size()!=dataidxsfold->size() )
	return false;

    return true;
}


#define mRet( event ) \
{ \
    if ( event == ErrorOccurred() ) \
	msg_ = rdr_->errMsg(); \
    return event; \
}


const TypeSet<int>& StratAmpCalc::attribIdxs() const
{
    auto* dataidxs = hp_dataidxs.getParam( this );
    return *dataidxs;
}


const TypeSet<int>& StratAmpCalc::foldAttribIdxs() const
{
    auto* dataidxsfold = hp_dataidxsfold.getParam( this );
    return *dataidxsfold;
}


bool StratAmpCalc::doOutputFold() const
{
    return outfold_;
}


int StratAmpCalc::nextStep()
{
    auto* selcomps = hp_selcomps.getParam( this );
    auto* dataidxs = hp_dataidxs.getParam( this );
    auto* dataidxsfold = hp_dataidxsfold.getParam( this );
    if ( ( !proc_ && !rdr_ ) || !tophorizon_ || dataidxs->isEmpty() )
	return ErrorOccurred();

    int res = -1;
    SeisTrc* trc = nullptr;
    if ( usesstored_ )
    {
	trc = new SeisTrc();
	const int rv = rdr_->get( trc->info() );
	if ( rv == 0 )
	    mRet( Finished() )
	else if ( rv == -1 )
	    mRet( ErrorOccurred() )

	if ( !rdr_->get(*trc) )
	    mRet( ErrorOccurred() );
    }
    else
    {
	res = proc_->nextStep();
	if ( res == 0 )
	    return Finished();

	if ( res == -1 )
	{
	    msg_ = proc_->uiMessage();
	    return ErrorOccurred();
	}

	trc = proc_->outputs_[0]->getTrc();
	if ( !trc )
	{
	    msg_ = proc_->uiMessage();
	    return ErrorOccurred();
	}
    }

    const BinID bid = trc->info().binID();
    for ( int idx=0; idx<dataidxs->size(); idx++ )
    {
	const EM::SubID subid = bid.toInt64();
	float z1 = tophorizon_->getZ( bid );
	float z2 = !bothorizon_ ? z1 : bothorizon_->getZ( bid );
	if ( mIsUdf(z1) || mIsUdf(z2) )
	    return MoreToDo();

	const StepInterval<float> zrg = trc->zRange();
	z1 += tophorshift_;
	const float snappedz1 = isclassification_
			      ? zrg.snap( z1, OD::SnapUpward ) : z1;

	z2 += bothorshift_;
	const float snappedz2 = isclassification_
			      ? zrg.snap( z2, OD::SnapDownward ) : z2;
	StepInterval<float> sampintv( snappedz1, snappedz2, zrg.step_ );
	sampintv.sort();
	sampintv.limitTo( zrg );

	Stats::CalcSetup rcsetup;
	rcsetup.require( stattyp_ );
	Stats::RunCalc<float> runcalc( rcsetup );
	for ( int sidx=0; sidx<sampintv.nrSteps()+1; sidx++ )
	{
	    const float zval = sampintv.atIndex( sidx );
	    const float val = trc->getValue( zval,
					     usesstored_ ? selcomps->get(idx)
							 : idx );
	    if ( !mIsUdf(val) )
		runcalc.addValue( val );
	}

	float outval = mUdf(float);
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

	const EM::Horizon3D& addtohor = addtotop_ ? *tophorizon_ : *bothorizon_;
	posid_.setSubID( subid );
	addtohor.auxdata.setAuxDataVal( dataidxs->get(idx), posid_, outval);
	if ( outfold_ )
	{
	    posidfold_.setSubID( subid );
	    addtohor.auxdata.setAuxDataVal( dataidxsfold->get(idx), posidfold_,
					      mCast(float,runcalc.count()) );
	}

    }

    if ( usesstored_ )
	delete trc;
    else
	proc_->outputs_[0]->deleteTrc();

    nrdone_++;
    return res || rdr_ ? MoreToDo() : Finished();
}


bool StratAmpCalc::saveAttribute( const EM::Horizon3D* hor, int attribidx,
				  bool overwrite, od_ostream* strm )
{
    auto* dataidxsfold = hp_dataidxsfold.getParam( this );
    return doSaveAttribute( *hor, attribidx, overwrite,	
			    dataidxsfold->first(), strm );
}


bool StratAmpCalc::doSaveAttribute( const EM::Horizon3D& hor, int attribidx,
				  bool overwrite, int foldidx,
				  od_ostream* strm )
{
    auto* dataidxsfold = hp_dataidxsfold.getParam( this );
    PtrMan<Executor> datasaver =
			hor.auxdata.auxDataSaver( attribidx, overwrite );
    if ( !(datasaver && datasaver->go(strm,false,false)) )
	return false;

    if ( outfold_ && dataidxsfold->isPresent(foldidx) )
    {
	datasaver.erase();
	datasaver = hor.auxdata.auxDataSaver( foldidx, overwrite);
	if ( !(datasaver && datasaver->go(strm,false,false)) )
	    return false;
    }

    return true;
}
