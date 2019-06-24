/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara Rao
 Date:		March 2008
________________________________________________________________________

-*/

#include "stratamp.h"

#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribstorprovider.h"
#include "trckeyzsampling.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "ioobj.h"
#include "odver.h"
#include "seisprovider.h"
#include "statruncalc.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "od_ostream.h"


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
    , prov_(0)
    , tophorizon_(tophor)
    , bothorizon_(bothor)
    , stattyp_(stattyp)
    , dataidx_(-1)
    , nrdone_(0)
    , usesstored_(false)
    , proc_(0)
    , hs_(hs)
    , outfold_(outputfold)
    , isclassification_(false)
    , tophorshift_(mUdf(float))
    , bothorshift_(mUdf(float))
{
    TrcKeyZSampling cs;
    cs.hsamp_ = hs;
    totnr_ = hs.nrInl() * hs.nrCrl();

    if ( tophor ) tophor->ref();
    if ( bothor ) bothor->ref();

    descset_ = new Attrib::DescSet( false );
}


StratAmpCalc::~StratAmpCalc()
{
    if ( tophorizon_ ) tophorizon_->unRef();
    if ( bothorizon_ ) bothorizon_->unRef();
    delete descset_; delete proc_; delete prov_;
}



#define mErrRet(s) { errmsg_ = s; return -1; }

int StratAmpCalc::init( const IOPar& pars )
{
    const uiString badinp = tr("Bad input");
    pars.get( sKeyTopShift(), tophorshift_ );
    pars.get( sKeyBottomShift(), bothorshift_ );
    if ( mIsUdf(tophorshift_) || mIsUdf(bothorshift_) )
	mErrRet( badinp )

    pars.getYN( sKeyIsClassification(), isclassification_ );

    addtotop_ = false;
    pars.getYN( sKeyAddToTopYN(), addtotop_ );
    const EM::Horizon3D* addtohor = addtotop_ ? tophorizon_ : bothorizon_;
    if ( !addtohor )
	mErrRet( badinp )

    //determine whether stored data is used
    PtrMan<IOPar> attribs = pars.subselect("Attributes");
    if ( !attribs || attribs->isEmpty() )
	mErrRet( badinp )
    uiRetVal uirv = descset_->usePar( *attribs );
    if ( !uirv.isOK() )
	mErrRet( uirv )

    BufferString outpstr = IOPar::compKey( sKey::Output(), 0 );
    PtrMan<IOPar> outputpar = pars.subselect( outpstr );
    if ( !outputpar || outputpar->isEmpty() )
	mErrRet( badinp )

    BufferString attribidstr = IOPar::compKey( sKey::Attributes(), 0 );
    int attribid;
    if ( !outputpar->get(attribidstr,attribid) )
	mErrRet( badinp )

    Attrib::Desc* targetdesc = descset_->getDesc( Attrib::DescID(attribid) );
    if ( !targetdesc )
	mErrRet( badinp )

    BufferString defstring;
    targetdesc->getDefStr( defstring );
    BufferString storstr = Attrib::StorageProvider::attribName();
    usesstored_ = storstr.isStartOf( defstring );
    if ( usesstored_ )
    {
	const StringPair strpair( targetdesc->getValParam(
		Attrib::StorageProvider::keyStr())->getStringValue(0) );
	const DBKey key( strpair.first() );

	prov_ = Seis::Provider::create( key, &uirv );
	if ( !prov_ )
	    errmsg_ =uirv;
	else
	{
	    TrcKeyZSampling tkzs;
	    tkzs.hsamp_ = hs_;
	    prov_->setSelData( new Seis::RangeSelData(tkzs) );
	}
    }
    else
    {
	PtrMan<Attrib::EngineMan> attrengman = new Attrib::EngineMan();
	proc_ = attrengman->usePar( pars, *descset_, uirv, 0 );
	if ( !proc_ )
	    mErrRet( badinp )
    }

    BufferString attribnm;
    pars.get( sKeyAttribName(), attribnm );
    if ( attribnm.isEmpty() )
	mErrRet( badinp )

    dataidx_ = addtohor->auxdata.auxDataIndex( attribnm );
    if ( dataidx_ < 0 )
	dataidx_ = addtohor->auxdata.addAuxData( attribnm );

    if ( outfold_ )
    {
	BufferString foldnm = attribnm;
	foldnm.add( "_fold" );
	dataidxfold_ = addtohor->auxdata.auxDataIndex( foldnm );
	if ( dataidxfold_ < 0 )
	    dataidxfold_ = addtohor->auxdata.addAuxData( foldnm );
    }

    return dataidx_;
}


uiString StratAmpCalc::message() const
{
    if ( errmsg_.isEmpty() )
	return uiStrings::phrHandling(uiStrings::sPosition(mPlural));
    return errmsg_;
}


#define mRet( event ) \
{ delete trc; return event; }

int StratAmpCalc::nextStep()
{
    if ( ( !proc_ && !prov_ ) || !tophorizon_ || dataidx_<0 )
	return ErrorOccurred();

    int res = -1;
    SeisTrc* trc;
    if ( usesstored_ )
    {
	trc = new SeisTrc;
	const uiRetVal uirv = prov_->getNext( *trc );
	if ( !uirv.isOK() )
	{
	    if ( isFinished(uirv) )
		mRet( Finished() );

	    errmsg_ = uirv;
	    mRet( ErrorOccurred() );
	}
    }
    else
    {
	res = proc_->nextStep();
	if ( res == 0 )
	    return Finished();
	if ( res == -1 )
	{
	    errmsg_ = proc_->message();
	    return ErrorOccurred();
	}

	trc = proc_->outputs_[0]->getTrc();
	if ( !trc )
	{
	    errmsg_ = tr("No trace processed");
	    return ErrorOccurred();
	}
    }

    const BinID bid = trc->info().binID();
    posid_ = EM::PosID::getFromRowCol( bid );
    float z1 = (float) tophorizon_->getPos(posid_).z_;
    float z2 = !bothorizon_ ? z1
	: (float) bothorizon_->getPos(posid_).z_;
    if ( mIsUdf(z1) || mIsUdf(z2) )
	return MoreToDo();

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
    addtohor->auxdata.setAuxDataVal( dataidx_, posid_, (float)outval );
    if ( outfold_ )
    {
	posidfold_ = posid_;
	addtohor->auxdata.setAuxDataVal( dataidxfold_, posidfold_,
					  mCast(float,runcalc.count()) );
    }

    nrdone_++;

    if ( usesstored_ )
	delete trc;
    else
	proc_->outputs_[0]->deleteTrc();

    return res || prov_ ? MoreToDo() : Finished();
}


bool StratAmpCalc::saveAttribute( const EM::Horizon3D* hor, int attribidx,
				  bool overwrite, od_ostream* strm )
{
    const uiString nosaver = tr("Cannot create output saver object");
    PtrMan<Executor> saver =
			hor->auxdata.auxDataSaver( attribidx, overwrite );
    if ( !(saver && saver->go(strm,false,false)) )
	{ errmsg_ = saver ? saver->message() : nosaver; return false; }

    if ( outfold_ )
    {
	saver.erase();
	saver = hor->auxdata.auxDataSaver( dataidxfold_, overwrite );
	if ( !(saver && saver->go(strm,false,false)) )
	    { errmsg_ = saver ? saver->message() : nosaver; return false; }
    }

    return true;
}
