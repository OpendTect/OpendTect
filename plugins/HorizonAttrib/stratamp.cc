/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara Rao
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: stratamp.cc,v 1.11 2011-03-03 13:32:12 cvshelene Exp $";

#include "stratamp.h"

#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribstorprovider.h"
#include "cubesampling.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "initalgo.h"
#include "initattributes.h"
#include "initprestackprocessing.h"
#include "ioman.h"
#include "ioobj.h"
#include "odver.h"
#include "seisread.h"
#include "statruncalc.h"
#include "seisselectionimpl.h"
#include "seistrc.h"


StratAmpCalc::StratAmpCalc( const EM::Horizon3D* tophor,
			    const EM::Horizon3D* bothor,
			    Stats::Type stattyp, const HorSampling& hs,
       			    bool outputfold )
    : Executor("Computing Stratal amplitude...")
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
{
    CubeSampling cs;
    cs.hrg = hs;
    totnr_ = hs.nrInl() * hs.nrCrl();

    if ( tophor ) tophor->ref();
    if ( bothor ) bothor->ref();

    descset_ = new Attrib::DescSet( false );
}


StratAmpCalc::~StratAmpCalc()
{
    if ( tophorizon_ ) tophorizon_->unRef();
    if ( bothorizon_ ) bothorizon_->unRef();
    if ( descset_ ) delete descset_;
    if ( proc_ ) delete proc_;
    if ( rdr_ ) delete rdr_;
}


int StratAmpCalc::init( const char* attribnm, bool addtotop, const IOPar& pars )
{
    addtotop_ = addtotop;
    const EM::Horizon3D* addtohor_ = addtotop ? tophorizon_ : bothorizon_;
    if ( !addtohor_ ) return -1;

    //determine whether stored data is used
    PtrMan<IOPar> attribs = pars.subselect("Attributes");
    float vsn = mODMajorVersion + 0.1*mODMinorVersion;
    if ( !attribs || !descset_->usePar( *attribs, vsn ) )
	return -1;

    BufferString outpstr = IOPar::compKey( sKey::Output, 0 );
    PtrMan<IOPar> outputpar = pars.subselect( outpstr );
    if ( !outputpar ) return -1;
    BufferString attribidstr = IOPar::compKey( sKey::Attributes, 0 );
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
	const LineKey lk( targetdesc->getValParam(
			Attrib::StorageProvider::keyStr())->getStringValue(0) );
	const MultiID key( lk.lineName() );
	const BufferString attrnm = lk.attrName();
	PtrMan<IOObj> seisobj = IOM().get( key );
	rdr_ = new SeisTrcReader( seisobj );
	CubeSampling cs;
	cs.hrg = hs_;
	rdr_->setSelData( new Seis::RangeSelData(cs) );
	rdr_->prepareWork();
    }
    else
    {
	Attributes::initStdClasses();
	Algo::initStdClasses();
	PreStackProcessing::initStdClasses();
	Attrib::StorageProvider::initClass();

	BufferString errmsg;
	BufferString linename; //TODO: function used in 2d?
	PtrMan<Attrib::EngineMan> attrengman = new Attrib::EngineMan();
	proc_ = attrengman->usePar( pars, *descset_, linename, errmsg );
	if ( !proc_ ) return -1;
    }

    dataidx_ = addtohor_->auxdata.auxDataIndex( attribnm );
    if ( dataidx_ < 0 ) dataidx_ = addtohor_->auxdata.addAuxData( attribnm );
    posid_.setObjectID( addtohor_->id() );
    posid_.setSectionID( addtohor_->sectionID(0) );

    if ( outfold_ )
    {
	BufferString foldnm = attribnm;
	foldnm += "_fold";
	dataidxfold_ = addtohor_->auxdata.auxDataIndex( foldnm );
	if ( dataidxfold_ < 0 )
	    dataidxfold_ = addtohor_->auxdata.addAuxData( foldnm );
	posidfold_.setObjectID( addtohor_->id() );
	posidfold_.setSectionID( addtohor_->sectionID(0) );
    }

    return dataidx_;
}


#define mRet( event ) \
{ delete trc; return event; }

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
	if ( res == -1 ) return Executor::ErrorOccurred();

	trc = proc_->outputs_[0]->getTrc();
	if ( !trc ) return Executor::ErrorOccurred();
    }

    const BinID bid = trc->info().binid;
    const EM::SubID subid = bid.toInt64();
    float z1 = tophorizon_->getPos(tophorizon_->sectionID(0),subid).z;
    float z2 = !bothorizon_ ? z1
		     : bothorizon_->getPos(bothorizon_->sectionID(0),subid).z;
    z1 += tophorshift_;
    z2 += bothorshift_;
    Interval<int> sampintv( trc->info().nearestSample(z1),
	    		    trc->info().nearestSample(z2) );
    sampintv.sort();

    if ( sampintv.start < 0 )
	sampintv.start = 0;
    if ( sampintv.stop >= trc->size() )
	sampintv.stop = trc->size()-1;

    Stats::RunCalcSetup rcsetup;
    rcsetup.require( stattyp_ );
    Stats::RunCalc<float> runcalc( rcsetup );
    for ( int idx=sampintv.start; idx<=sampintv.stop; idx++ )
    {
	const float val = trc->get( idx, 0 );
	if ( !mIsUdf(val) )
	    runcalc.addValue( val );
    }

    float outval = mUdf( float );
    switch ( stattyp_ )
    {
	case Stats::Min: outval = runcalc.min(); break;
	case Stats::Max: outval = runcalc.max(); break;
	case Stats::Average: outval = runcalc.average(); break;
	case Stats::RMS: outval = runcalc.rms(); break;  
	case Stats::Sum: outval = runcalc.sum(); break;  
	default: break;
    }

    const EM::Horizon3D* addtohor_ = addtotop_ ? tophorizon_ : bothorizon_;
    posid_.setSubID( subid );
    addtohor_->auxdata.setAuxDataVal( dataidx_, posid_, outval );

    if ( outfold_ )
    {
	posidfold_.setSubID( subid );
	addtohor_->auxdata.setAuxDataVal( dataidxfold_, posidfold_,
					  runcalc.count() );
    }

    nrdone_++;

    if ( usesstored_ )
	delete trc;
    else
	proc_->outputs_[0]->deleteTrc();

    return res || rdr_ ? Executor::MoreToDo() : Executor::Finished();
}
