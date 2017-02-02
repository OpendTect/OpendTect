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
#include "dbman.h"
#include "ioobj.h"
#include "odver.h"
#include "seisprovider.h"
#include "statruncalc.h"
#include "seisselectionimpl.h"
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

StratAmpCalc::StratAmpCalc( const EM::Horizon3D* tophor,
			    const EM::Horizon3D* bothor,
			    Stats::Type stattyp, const TrcKeySampling& hs,
			    bool outputfold )
    : Executor("Computing Stratal amplitude...")
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


int StratAmpCalc::init( const IOPar& pars )
{
    pars.get( sKeyTopShift(), tophorshift_ );
    pars.get( sKeyBottomShift(), bothorshift_ );
    if ( mIsUdf(tophorshift_) || mIsUdf(bothorshift_) )
	return -1;

    addtotop_ = false;
    pars.getYN( sKeyAddToTopYN(), addtotop_ );
    const EM::Horizon3D* addtohor = addtotop_ ? tophorizon_ : bothorizon_;
    if ( !addtohor ) return -1;

    //determine whether stored data is used
    PtrMan<IOPar> attribs = pars.subselect("Attributes");
    if ( !attribs || !descset_->usePar(*attribs) )
	return -1;

    BufferString outpstr = IOPar::compKey( sKey::Output(), 0 );
    PtrMan<IOPar> outputpar = pars.subselect( outpstr );
    if ( !outputpar ) return -1;
    BufferString attribidstr = IOPar::compKey( sKey::Attributes(), 0 );
    int attribid;
    if ( !outputpar->get(attribidstr,attribid) ) return -1;

    Attrib::Desc* targetdesc =
			descset_->getDesc( Attrib::DescID(attribid,false) );
    if ( !targetdesc ) return -1;

    BufferString defstring;
    targetdesc->getDefStr( defstring );
    BufferString storstr = Attrib::StorageProvider::attribName();
    usesstored_ = storstr.isStartOf( defstring );
    if ( usesstored_)
    {
	const StringPair strpair( targetdesc->getValParam(
		Attrib::StorageProvider::keyStr())->getStringValue(0) );
	const DBKey key = DBKey::getFromString( strpair.first() );

	uiRetVal uirv;
	prov_ = Seis::Provider::create( key, &uirv );
	if ( !prov_ )
	    errmsg_ = uirv;
	else
	{
	    TrcKeyZSampling tkzs;
	    tkzs.hsamp_ = hs_;
	    prov_->setSelData( new Seis::RangeSelData(tkzs) );
	}
    }
    else
    {
	uiString errmsg;
	PtrMan<Attrib::EngineMan> attrengman = new Attrib::EngineMan();
	proc_ = attrengman->usePar( pars, *descset_, 0, errmsg, 0 );
	if ( !proc_ ) return -1;
    }

    BufferString attribnm;
    pars.get( sKeyAttribName(), attribnm );
    if ( attribnm.isEmpty() ) return -1;

    dataidx_ = addtohor->auxdata.auxDataIndex( attribnm );
    if ( dataidx_ < 0 ) dataidx_ = addtohor->auxdata.addAuxData( attribnm );

    posid_.setObjectID( addtohor->id() );
    posid_.setSectionID( addtohor->sectionID(0) );
    if ( outfold_ )
    {
	BufferString foldnm = attribnm;
	foldnm.add( "_fold" );
	dataidxfold_ = addtohor->auxdata.auxDataIndex( foldnm );
	if ( dataidxfold_ < 0 )
	    dataidxfold_ = addtohor->auxdata.addAuxData( foldnm );

	posidfold_.setObjectID( addtohor->id() );
	posidfold_.setSectionID( addtohor->sectionID(0) );
    }

    return dataidx_;
}


uiString StratAmpCalc::message() const
{
    return !errmsg_.isEmpty() ? errmsg_
		: uiStrings::phrHandling(uiStrings::sPosition(mPlural));
}


#define mRet( event ) \
{ delete trc; return event; }

int StratAmpCalc::nextStep()
{
    if ( ( !proc_ && !prov_ ) || !tophorizon_ || dataidx_<0 )
	return Executor::ErrorOccurred();

    int res = -1;
    SeisTrc* trc;
    if ( usesstored_ )
    {
	trc = new SeisTrc();
	const uiRetVal uirv = prov_->getNext( *trc );
	if ( !uirv.isOK() )
	{
	    if ( isFinished(uirv) )
		mRet( Executor::Finished() );

	    errmsg_ = uirv;
	    mRet( Executor::ErrorOccurred() );
	}
    }
    else
    {
	res = proc_->nextStep();
	if ( res == 0 ) return Executor::Finished();
	if ( res == -1 ) return Executor::ErrorOccurred();

	trc = proc_->outputs_[0]->getTrc();
	if ( !trc ) return Executor::ErrorOccurred();
    }

    const BinID bid = trc->info().binID();
    const EM::SubID subid = bid.toInt64();
    float z1 = (float) tophorizon_->getPos(tophorizon_->sectionID(0),subid).z_;
    float z2 = !bothorizon_ ? z1
	: (float) bothorizon_->getPos(bothorizon_->sectionID(0),subid).z_;
    if ( mIsUdf(z1) || mIsUdf(z2) )
	return Executor::MoreToDo();

    z1 += tophorshift_;
    z2 += bothorshift_;
    StepInterval<float> sampintv( z1, z2, trc->info().sampling_.step );
    sampintv.sort();
    sampintv.limitTo( trc->zRange() );

    Stats::CalcSetup rcsetup;
    rcsetup.require( stattyp_ );
    Stats::RunCalc<float> runcalc( rcsetup );
    for ( float zval=sampintv.start; zval<=sampintv.stop; zval+=sampintv.step )
    {
	const float val = trc->getValue( zval, 0 );
	if ( !mIsUdf(val) )
	    runcalc.addValue( val );
    }

    float outval = mUdf( float );
    switch ( stattyp_ )
    {
	case Stats::Min: outval = runcalc.min(); break;
	case Stats::Max: outval = runcalc.max(); break;
	case Stats::Average: outval = (float) runcalc.average(); break;
	case Stats::RMS: outval = (float) runcalc.rms(); break;
	case Stats::Sum: outval = runcalc.sum(); break;
	default: break;
    }

    const EM::Horizon3D* addtohor = addtotop_ ? tophorizon_ : bothorizon_;
    posid_.setSubID( subid );
    addtohor->auxdata.setAuxDataVal( dataidx_, posid_, (float) outval );
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

    return res || prov_ ? Executor::MoreToDo() : Executor::Finished();
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
