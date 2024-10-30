/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribengman.h"

#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribprocessor.h"
#include "attribprovider.h"
#include "attribstorprovider.h"
#include "attribparambase.h"

#include "convmemvalseries.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "linesetposinfo.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "seis2ddata.h"
#include "seistrc.h"
#include "survgeom2d.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "zdomain.h"

#include <string.h>

namespace Attrib
{

EngineMan::EngineMan()
    : tkzs_(*new TrcKeyZSampling)
    , geomid_(Pos::GeomID::udf())
    , dpm_(DPM(DataPackMgr::SeisID()))
{
}


EngineMan::~EngineMan()
{
    delete procattrset_;
    delete inpattrset_;
    delete nlamodel_;
    delete &tkzs_;
}


bool EngineMan::getPossibleVolume( DescSet& attribset, TrcKeyZSampling& cs,
				   const char* linename, const DescID& outid )
{
    TypeSet<DescID> desiredids(1,outid);

    uiString errmsg;
    DescID evalid = createEvaluateADS( attribset, desiredids, errmsg );
    PtrMan<Processor> proc =
			createProcessor( attribset, linename, evalid, errmsg );
    if ( !proc ) return false;

    proc->computeAndSetRefZStepAndZ0();
    proc->getProvider()->setDesiredVolume( cs );
    return proc->getProvider()->getPossibleVolume( -1, cs );
}


Processor* EngineMan::usePar( const IOPar& iopar, DescSet& attribset,
			      const char* linename, uiString& errmsg )
{
    int outputidx = 0;
    TypeSet<DescID> ids;
    while ( true )
    {
	BufferString outpstr = IOPar::compKey( sKey::Output(), outputidx );
	PtrMan<IOPar> outputpar = iopar.subselect( outpstr );
	if ( !outputpar )
	{
	    if ( !outputidx )
	    { outputidx++; continue; }
	    else
		break;
	}

	int attribidx = 0;
	while ( true )
	{
	    BufferString attribidstr =
			IOPar::compKey( sKey::Attributes(), attribidx );
	    int attribid;
	    if ( !outputpar->get(attribidstr,attribid) )
		break;

	    ids += DescID(attribid,false);
	    attribidx++;
	}

	outputidx++;
    }

    DescID evalid = createEvaluateADS( attribset, ids, errmsg );
    Processor* proc = createProcessor( attribset, linename, evalid, errmsg );
    if ( !proc )
	return nullptr;

    for ( int idx=1; idx<ids.size(); idx++ )
	proc->addOutputInterest(idx);

    PtrMan<IOPar> outpar =
	iopar.subselect( IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( !outpar || !tkzs_.usePar(*outpar) )
    {
	if ( !attribset.is2D() )
	    tkzs_.init();
	else
	{
	    // doesn't make much sense, but is better than nothing
	    tkzs_.set2DDef();

	    const Pos::GeomID geomid = Survey::GM().getGeomID( linename );
	    tkzs_.hsamp_.setGeomID( geomid );
	    if ( outpar && outpar->hasKey(sKey::TrcRange()) )
	    {
		StepInterval<int> trcrg( 0, 0, 1 );
		outpar->get( sKey::TrcRange(), trcrg );
		tkzs_.hsamp_.setCrlRange( trcrg );
		outpar->get( sKey::ZRange(), tkzs_.zsamp_ );
	    }
	    else
	    {
		mDynamicCastGet( const Survey::Geometry2D*, geom2d,
				 Survey::GM().getGeometry(geomid) );
		if ( geom2d )
		{
		    tkzs_.hsamp_.setCrlRange( geom2d->data().trcNrRange() );
		    tkzs_.zsamp_ = geom2d->data().zRange();
		}
	    }
	}
    }

    const Pos::GeomID geomid = Survey::GM().getGeomID( linename );
    RefMan<SeisTrcStorOutput> storeoutp =
				createOutput( iopar, geomid, errmsg );
    if ( !storeoutp )
	return nullptr;

    bool exttrctosi;
    BufferString basekey = IOPar::compKey( "Output",0 );
    if ( iopar.getYN( IOPar::compKey( basekey,SeisTrc::sKeyExtTrcToSI() ),
		      exttrctosi) )
	storeoutp->setTrcGrow( exttrctosi );

    if ( storeoutp->getOutpNames().isEmpty() )
    {
	BufferStringSet outnms;
	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    const StringPair userref( attribset.getDesc(ids[idx])->userRef() );
	    outnms.add(
		userref.hasSecond() ? userref.second().buf() : userref.buf() );
	}

	storeoutp->setOutpNames( outnms );
    }

    proc->addOutput( storeoutp.ptr() );
    return proc;
}


Processor* EngineMan::createProcessor( const DescSet& attribset,
				       const char* linename,const DescID& outid,
				       uiString& errmsg )
{
    ConstRefMan<Desc> tdesc = attribset.getDesc( outid );
    Desc* targetdesc = tdesc.getNonConstPtr();
    if ( !targetdesc )
	return nullptr;

    targetdesc->updateParams();
    auto* processor = new Processor( *targetdesc, linename, errmsg );
    if ( !processor->isOK() )
    {
	delete processor;
	return nullptr;
    }

    processor->addOutputInterest( targetdesc->selectedOutput() );
    return processor;
}


void EngineMan::setExecutorName( Executor* ex )
{
    if ( !ex ) return;

    BufferString usernm( getCurUserRef() );
    if ( usernm.isEmpty() || !inpattrset_ ) return;

    if ( curattridx_ < 0 || curattridx_ >= attrspecs_.size() )
	ex->setName( "Processing attribute" );

    SelSpec& ss = attrspecs_[curattridx_];
    BufferString nm( "Calculating " );
    if ( ss.isNLA() && nlamodel_ )
    {
	nm = "Applying ";
	nm += nlamodel_->nlaType(true);
	nm += ": calculating";
	if ( IOObj::isKey(usernm) )
	    usernm = IOM().nameOf( usernm.buf() );
    }
    else
    {
	ConstRefMan<Desc> desc = inpattrset_->getDesc( ss.id() );
	if ( desc && desc->isStored() )
	    nm = "Reading from";
    }

    nm += " \"";
    nm += usernm;
    nm += "\"";

    ex->setName( nm );
}


RefMan<SeisTrcStorOutput> EngineMan::createOutput( const IOPar& pars,
						   const Pos::GeomID& geomid,
						   uiString& errmsg )
{
    const BufferString typestr =
		pars.find( IOPar::compKey(sKey::Output(),sKey::Type()) );
    if ( !typestr.isEqual(sKey::Cube()) )
	return nullptr;

    RefMan<SeisTrcStorOutput> outp = new SeisTrcStorOutput( tkzs_, geomid );
    outp->setGeometry( tkzs_ );
    const bool res = outp->doUsePar( pars );
    if ( !res )
    {
	errmsg = outp->errMsg();
	return nullptr;
    }

    return outp;
}


void EngineMan::setNLAModel( const NLAModel* model )
{
    deleteAndNullPtr( nlamodel_ );
    if ( model )
	nlamodel_ = model->clone();
}


void EngineMan::setAttribSet( const DescSet* ads )
{
    deleteAndNullPtr( inpattrset_ );
    if ( ads )
	inpattrset_ = new DescSet( *ads );
}


const char* EngineMan::getCurUserRef() const
{
    const int idx = curattridx_;
    if ( attrspecs_.isEmpty() || !attrspecs_[idx].id().isValid() ) return "";

    SelSpec& ss = const_cast<EngineMan*>(this)->attrspecs_[idx];
    if ( attrspecs_[idx].isNLA() )
    {
	if ( !nlamodel_ ) return "";
	ss.setRefFromID( *nlamodel_ );
    }
    else
    {
	if ( !inpattrset_ ) return "";
	ss.setRefFromID( *inpattrset_ );
    }
    return attrspecs_[idx].userRef();
}


RefMan<RegularSeisDataPack> EngineMan::getDataPackOutput(const Processor& proc)
{
    RefMan<RegularSeisDataPack> output;
    if ( proc.outputs_.size()==1 && !cache_ )
    {
	output = const_cast<RegularSeisDataPack*>(
			proc.outputs_[0]->getDataPack() );
	if ( !output || !output->sampling().isDefined() )
	    return nullptr;

	for ( int idx=0; idx<attrspecs_.size(); idx++ )
	    output->setComponentName( attrspecs_[idx].userRef(), idx );

	const Attrib::SelSpec& attrspec = attrspecs_.first();
	const ZDomain::Def& zddef = ZDomain::Def::get( attrspec.zDomainKey() );
	const ZDomain::Info zdomain( zddef, attrspec.zDomainUnit() );
	output->setZDomain( zdomain );
	output->setName( attrspec.userRef() );
	return output;
    }

    RefObjectSet<const RegularSeisDataPack> packset;
    for ( int idx=0; idx<proc.outputs_.size(); idx++ )
    {
	ConstRefMan<RegularSeisDataPack> dp = proc.outputs_[idx] ?
					    proc.outputs_[idx]->getDataPack()
					    : nullptr;
	if ( !dp || !dp->sampling().isDefined() )
	    continue;

	dpm_.add( dp.ptr() );
	if ( packset.size() && packset[0]->nrComponents()!=dp->nrComponents() )
	    continue;

	packset += dp.ptr();
    }

    if ( cache_ )
    {
	packset += cache_.ptr();
	if ( !dpm_.isPresent(cache_->id()) )
	    dpm_.add( cache_.ptr() );
    }

    if ( !packset.isEmpty() )
	output = const_cast<RegularSeisDataPack*>(
					    getDataPackOutput(packset).ptr() );

    return output;
}



class DataPackCopier : public ParallelTask
{
public:

    typedef const unsigned char* ConstCompPtr;
    typedef unsigned char* CompPtr;

DataPackCopier( const RegularSeisDataPack& in, RegularSeisDataPack& out )
    : in_(in), out_(out)
    , domemcopy_(false)
    , samplebytes_(sizeof(float))
    , intracebytes_(0)
    , inlinebytes_(0)
    , outtracebytes_(0)
    , outlinebytes_(0)
    , bytestocopy_(0)
{
    worktkzs_ = out.sampling();
    if ( !in_.sampling().getIntersection(out.sampling(),worktkzs_,true) )
	worktkzs_.setEmpty();

    totalnr_ = worktkzs_.hsamp_.totalNr();

    const int nrcomps = out_.nrComponents();
    incomp_ = new int[nrcomps];
    inptr_ = new ConstCompPtr[nrcomps];
    outptr_ = new CompPtr[nrcomps];
}

~DataPackCopier()
{
    delete [] incomp_;
    delete [] inptr_;
    delete [] outptr_;
}

od_int64 nrIterations() const override		{ return totalnr_; }

bool doPrepare( int nrthreads ) override
{
    if ( in_.isEmpty() || out_.isEmpty() )
	return false;

    if ( totalnr_ == 0 )
	return true;

    for ( int idc=0; idc<out_.nrComponents(); idc++ )
    {
	incomp_[idc] = in_.getComponentIdx( out_.getComponentName(idc), idc );
	if ( incomp_[idc] < 0 )
	    return false;
    }

    if ( in_.getDataDesc() != out_.getDataDesc() )
	return true;

    for ( int idc=0; idc<out_.nrComponents(); idc++ )
    {
	inptr_[idc] = mCast( ConstCompPtr, in_.data(incomp_[idc]).getData() );
	mDynamicCastGet( const ConvMemValueSeries<float>*, instorage,
			 in_.data(incomp_[idc]).getStorage() );
	if ( instorage )
	{
	    inptr_[idc] = mCast( ConstCompPtr, instorage->storArr() );
	    samplebytes_ = in_.getDataDesc().nrBytes();
	}

	outptr_[idc] = mCast( CompPtr, out_.data(idc).getData() );
	mDynamicCastGet( const ConvMemValueSeries<float>*, outstorage,
			 out_.data(idc).getStorage() );
	if ( outstorage )
	    outptr_[idc] = mCast( CompPtr, outstorage->storArr() );

	if ( !inptr_[idc] || !outptr_[idc] )
	    return true;

        const float start = worktkzs_.zsamp_.start_;
	inptr_[idc] += samplebytes_ * in_.zRange().nearestIndex( start );
	outptr_[idc] += samplebytes_ * out_.zRange().nearestIndex( start );
    }

    intracebytes_ = samplebytes_ * in_.sampling().size(TrcKeyZSampling::Z);
    inlinebytes_ = intracebytes_ * in_.sampling().size(TrcKeyZSampling::Crl);

    outtracebytes_ = samplebytes_ * out_.sampling().size(TrcKeyZSampling::Z);
    outlinebytes_ = outtracebytes_ * out_.sampling().size(TrcKeyZSampling::Crl);

    bytestocopy_ = samplebytes_ * worktkzs_.nrZ();

    domemcopy_ = true;
    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    if ( totalnr_ == 0 )
	return true;

    const TrcKeySampling intks = in_.sampling().hsamp_;
    const TrcKeySampling outtks = out_.sampling().hsamp_;
    const TrcKeySampling worktks = worktkzs_.hsamp_;

    const StepInterval<float> inzsamp = in_.sampling().zsamp_;
    const StepInterval<float> outzsamp = out_.sampling().zsamp_;
    const StepInterval<float> workzsamp = worktkzs_.zsamp_;

    const od_int64 intracebytes = intracebytes_;
    const od_int64 inlinebytes = inlinebytes_;
    const od_int64 outtracebytes = outtracebytes_;
    const od_int64 outlinebytes = outlinebytes_;

    const od_int64 bytestocopy = bytestocopy_;
    const int samplebytes = samplebytes_;

    const int nrcomps = out_.nrComponents();
    const bool domemcopy = domemcopy_;

    int nrz = worktkzs_.nrZ();
    int* inzlut = 0;
    int* outzlut = 0;

    if ( !domemcopy || !mIsEqual(inzsamp.step_, outzsamp.step_, mDefEpsF) )
    {
	inzlut = new int[ nrz ];
	outzlut = new int[ nrz ];

	const int nrinpzsamp = inzsamp.nrSteps()+1;
	const int nroutzsamp = outzsamp.nrSteps()+1;
	for ( int idz=0; idz<nrz; idz++ )
	{
	    const float zval = workzsamp.atIndex( idz );
	    inzlut[idz] = inzsamp.nearestIndex( zval );
	    outzlut[idz] = outzsamp.nearestIndex( zval );

	    if ( inzlut[idz]>=nrinpzsamp || outzlut[idz]>=nroutzsamp )
	    {
		nrz = idz;
		break;
	    }
	}

	for ( int idz=nrz-1; domemcopy && idz>=0; idz-- )
	{
	    // Need relative offsets in case of domemcopy
	    inzlut[idz] =  idz ? samplebytes * (inzlut[idz]-inzlut[idz-1]) : 0;
	    outzlut[idz] = idz ? samplebytes*(outzlut[idz]-outzlut[idz-1]) : 0;
	}
    }

    const int nrtrcs = worktks.nrTrcs();
    int inlidx = mCast( int, start/nrtrcs );
    int crlidx = mCast( int, start%nrtrcs );

    const int halfinlstep = intks.step_.lineNr() / 2;
    BinID bid = worktks.atIndex( inlidx, 0 );
    int outinlidx = outtks.lineIdx( bid.inl() );
    int shiftedtogetnearestinl = bid.lineNr() + halfinlstep;
    int ininlidx = intks.lineIdx( shiftedtogetnearestinl );

    int* incrllut = new int[ nrtrcs ];
    int* outcrllut = new int[ nrtrcs ];
    const int halfcrlstep = intks.step_.trcNr() / 2;
    for ( int cidx=0; cidx<nrtrcs; cidx++ )
    {
	bid = worktks.atIndex( 0, cidx );
	outcrllut[cidx] = outtks.trcIdx( bid.crl() );
	const int shiftedtogetnearestcrl = bid.trcNr() + halfcrlstep;
	incrllut[cidx] = intks.trcIdx( shiftedtogetnearestcrl );
    }

    for ( od_int64 gidx=start; gidx<=stop; gidx++ )
    {
	const int outcrlidx = outcrllut[crlidx];
	const int incrlidx = incrllut[crlidx];

	if ( domemcopy )
	{
	    const od_int64 inoffset = ininlidx*inlinebytes +
				      incrlidx*intracebytes;
	    const od_int64 outoffset = outinlidx*outlinebytes +
				       outcrlidx*outtracebytes;

	    for ( int idc=0; idc<nrcomps; idc++ )
	    {
		const unsigned char* curinptr = inptr_[idc] + inoffset;
		unsigned char* curoutptr = outptr_[idc] + outoffset;
		if ( inzlut && outzlut )
		{
		    for ( int idz=0; idz<nrz; idz++ )
		    {
			curinptr += inzlut[idz];
			curoutptr += outzlut[idz];
			OD::sysMemCopy( curoutptr, curinptr, samplebytes );
		    }
		}
		else
		    OD::sysMemCopy( curoutptr, curinptr, bytestocopy );

	    }
	}
	else
	{
	    for ( int idz=0; idz<nrz; idz++ )
	    {
		const int inzidx = inzlut[idz];
		const int outzidx = outzlut[idz];

		for ( int idc=0; idc<nrcomps; idc++ )
		{
		    const float val =
			in_.data(incomp_[idc]).get( ininlidx, incrlidx, inzidx);
		    if ( mFastIsFloatDefined(val) )
			out_.data(idc).set( outinlidx, outcrlidx, outzidx, val);
		}
	    }
	}

	if ( ++crlidx >= nrtrcs )
	{
	    crlidx = 0;
	    bid = worktks.atIndex( ++inlidx, 0 );
	    outinlidx = outtks.lineIdx( bid.inl() );
	    shiftedtogetnearestinl = bid.lineNr() + halfinlstep;
	    ininlidx = intks.lineIdx( shiftedtogetnearestinl );
	}
    }

    delete [] incrllut;
    delete [] outcrllut;
    if ( inzlut )  delete [] inzlut;
    if ( outzlut ) delete [] outzlut;

    return true;
}


protected:
    const RegularSeisDataPack&	in_;
    RegularSeisDataPack&	out_;
    TrcKeyZSampling		worktkzs_;

    od_int64			totalnr_;
    bool			domemcopy_;

    int				samplebytes_;
    int*			incomp_;
    ConstCompPtr*		inptr_;
    CompPtr*			outptr_;
    od_int64			intracebytes_;
    od_int64			inlinebytes_;
    od_int64			outtracebytes_;
    od_int64			outlinebytes_;
    od_int64			bytestocopy_;
};


RefMan<RegularSeisDataPack> EngineMan::getDataPackOutput(
			const ObjectSet<const RegularSeisDataPack>& packset )
{
    if ( packset.isEmpty() ) return nullptr;
    const char* category = SeisDataPack::categoryStr(
			tkzs_.defaultDir()!=TrcKeyZSampling::Z,
			tkzs_.is2D() );
    RefMan<RegularSeisDataPack> output =
		new RegularSeisDataPack(category,&packset[0]->getDataDesc());
    if ( packset[0]->getScaler() )
	output->setScaler( *packset[0]->getScaler() );

    if ( cache_ && cache_->sampling().zsamp_.step_ != tkzs_.zsamp_.step_ )
    {
	TrcKeyZSampling cswithcachestep = tkzs_;
        cswithcachestep.zsamp_.step_ = cache_->sampling().zsamp_.step_;
	output->setSampling( cswithcachestep );
    }
    else
    {
	// For running dimensions, steps of TrcKeyZSampling (Inl, Crl and Z)
	// should be same as that of datapack (packset) that is preloaded.
	TrcKeyZSampling availabletkzs = packset[0]->sampling();
	for ( int idx=1; idx<packset.size(); idx++ )
	    availabletkzs.include( packset[idx]->sampling() );

	TrcKeyZSampling outputtkzs = tkzs_;
	outputtkzs.adjustTo( availabletkzs, true );
	output->setSampling( outputtkzs );
    }

    for ( int idx=0; idx<attrspecs_.size(); idx++ )
    {
	const char* compnm = attrspecs_[idx].userRef();
	if ( packset[0]->getComponentIdx(compnm,idx) >= 0 )
	    output->addComponent( compnm );
    }

    const Attrib::SelSpec& attrspec = attrspecs_.first();
    const ZDomain::Def& zddef = ZDomain::Def::get( attrspec.zDomainKey() );
    const ZDomain::Info zdomain( zddef, attrspec.zDomainUnit() );
    output->setZDomain( zdomain );
    output->setName( attrspec.userRef() );

    for ( const auto* regsdp : packset )
    {
	DataPackCopier copier( *regsdp, *output );
	copier.execute();
    }

    return output;
}


void EngineMan::setAttribSpecs( const TypeSet<SelSpec>& specs )
{ attrspecs_ = specs; }


void EngineMan::setAttribSpec( const SelSpec& spec )
{
    attrspecs_.erase();
    attrspecs_ += spec;
}


void EngineMan::setTrcKeyZSampling( const TrcKeyZSampling& newcs )
{
    tkzs_ = newcs;
    tkzs_.normalize();

    setGeomID( newcs.hsamp_.getGeomID() );
}


DescSet* EngineMan::createNLAADS( DescID& nladescid, uiString& errmsg,
				  const DescSet* addtoset )
{
    if ( !nlamodel_ )
    { errmsg = toUiString("Internal: No NLA Model"); return 0; }

    if ( attrspecs_.isEmpty() ) return 0;
    DescSet* descset = addtoset ? new DescSet( *addtoset )
				: new DescSet( attrspecs_[0].is2D() );

    if ( !addtoset && !descset->usePar(nlamodel_->pars()) )
    {
	errmsg = descset->errMsg();
	delete descset;
	return 0;
    }

    BufferString s;
    nlamodel_->dump(s);
    BufferString defstr( nlamodel_->nlaType(true) );
    defstr += " specification=\""; defstr += s; defstr += "\"";

    addNLADesc( defstr, nladescid, *descset, attrspecs_[0].id().asInt(),
		nlamodel_, errmsg );

    DescSet* cleanset = descset->optimizeClone( nladescid );
    delete descset;
    return cleanset;
}


void EngineMan::addNLADesc( const char* specstr, DescID& nladescid,
			    DescSet& descset, int outputnr,
			    const NLAModel* nlamdl, uiString& errmsg )
{
    RefMan<Desc> desc = PF().createDescCopy( "NN" );
    desc->setDescSet( &descset );
    if ( !desc->parseDefStr(specstr) )
    {
	errmsg = tr("Invalid definition string for NLA model:\n%1")
		    .arg( specstr );
	return;
    }

    desc->setHidden( true );

    // Need to make a Provider because the inputs and outputs may
    // not be known otherwise
    errmsg = Provider::prepare( *desc );
    if ( !errmsg.isEmpty() )
	{ return; }

    const int nrinputs = desc->nrInputs();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	const char* inpname = desc->inputSpec(idx).getDesc();
	DescID descid = descset.getID( inpname, true );
	if ( !descid.isValid() )
	{
	    descid = descset.getID( inpname, false );
	    if ( !descid.isValid() )
	    {
		// It could be 'storage', but it's not yet in the set ...
		PtrMan<IOObj> ioobj;
		if ( IOObj::isKey(inpname) )
		{
		    MultiID key;
		    key.fromString( inpname );
		    ioobj = IOM().get( key );
		}
		else
		{
		    BufferString rawnmbufstr;
	    //because constructor has strange behavior with embeded strings
		    rawnmbufstr += inpname;
		    rawnmbufstr.unEmbed( '[', ']' );
		    if ( rawnmbufstr.buf() && inpname &&
			 strcmp( rawnmbufstr.buf(), inpname ) )
		    {
			const char* tgname = descset.is2D() ? "2D Seismic Data"
							    : "Seismic Data";
			ioobj = IOM().get( rawnmbufstr.buf(), tgname );
		    }
		}
		if ( ioobj )
		{
		    RefMan<Desc> stordesc =
			PF().createDescCopy( StorageProvider::attribName() );
		    stordesc->setDescSet( &descset );
		    ValParam* idpar =
			stordesc->getValParam( StorageProvider::keyStr() );
		    idpar->setValue( ioobj->key() );
		    stordesc->setUserRef( ioobj->name() );
		    descid = descset.addDesc( stordesc.ptr() );
		    if ( !descid.isValid() )
		    {
			errmsg = tr("NLA input '%1' cannot be found in "
				    "the provided set.").arg( inpname );
			return;
		    }
		}
	    }
	}

	desc->setInput( idx, descset.getDesc(descid).ptr() );
    }

    if ( outputnr > desc->nrOutputs() )
    {
	errmsg = tr("Output %1 not present.").arg( toString(outputnr) );
	return;
    }

    const NLADesign& nlades = nlamdl->design();
    desc->setUserRef( *nlades.outputs_[outputnr] );
    desc->selectOutput( outputnr );

    nladescid = descset.addDesc( desc.ptr() );
    if ( nladescid == DescID::undef() )
	errmsg = descset.errMsg();
}


DescID EngineMan::createEvaluateADS( DescSet& descset,
				     const TypeSet<DescID>& outids,
				     uiString& errmsg )
{
    if ( outids.isEmpty() ) return DescID::undef();
    if ( outids.size() == 1 ) return outids[0];

    RefMan<Desc> desc = PF().createDescCopy( "Evaluate" );
    if ( !desc )
	return DescID::undef();

    desc->setDescSet( &descset );
    desc->setNrOutputs( Seis::UnknowData, outids.size() );

    desc->setHidden( true );

    const int nrinputs = outids.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	desc->addInput( InputSpec("Data",true) );
	RefMan<Desc> inpdesc = descset.getDesc( outids[idx] );
	if ( !inpdesc )
	    continue;

	desc->setInput( idx, inpdesc.ptr() );
    }

    desc->setUserRef( "evaluate attributes" );
    desc->selectOutput(0);

    const DescID evaldescid = descset.addDesc( desc.ptr() );
    if ( evaldescid == DescID::undef() )
	errmsg = descset.errMsg();

    return evaldescid;
}


#define mStepEps 1e-3


Processor* EngineMan::createScreenOutput2D( uiString& errmsg,
					    Data2DHolder& output )
{
    Processor* proc = getProcessor( errmsg );
    if ( !proc )
	return 0;

    Interval<int> trcrg( tkzs_.hsamp_.start_.crl(), tkzs_.hsamp_.stop_.crl() );
    Interval<float> zrg( tkzs_.zsamp_.start_, tkzs_.zsamp_.stop_ );

    TwoDOutput* attrout = new TwoDOutput( trcrg, zrg, geomid_ );
    attrout->setOutput( output );
    proc->addOutput( attrout );

    return proc;
}

#define mRg(dir) (cachecs.dir##samp_)

Processor* EngineMan::createDataPackOutput( uiString& errmsg,
					    const RegularSeisDataPack* prev )
{
    cache_ = nullptr;
    if ( tkzs_.isEmpty() )
	prev = nullptr;
    else if ( prev )
    {
	cache_ = prev;
	const TrcKeyZSampling cachecs = cache_->sampling();
	if ( mRg(h).step_ != tkzs_.hsamp_.step_
	  || (mRg(h).start_.inl() - tkzs_.hsamp_.start_.inl()) %
		tkzs_.hsamp_.step_.inl()
	  || (mRg(h).start_.crl() - tkzs_.hsamp_.start_.crl()) %
		tkzs_.hsamp_.step_.crl()
	  || mRg(h).start_.inl() > tkzs_.hsamp_.stop_.inl()
	  || mRg(h).stop_.inl() < tkzs_.hsamp_.start_.inl()
	  || mRg(h).start_.crl() > tkzs_.hsamp_.stop_.crl()
	  || mRg(h).stop_.crl() < tkzs_.hsamp_.start_.crl()
             || mRg(z).start_ > tkzs_.zsamp_.stop_ + mStepEps*tkzs_.zsamp_.step_
             || mRg(z).stop_ < tkzs_.zsamp_.start_ - mStepEps*tkzs_.zsamp_.step_ )
	    // No overlap, gotta crunch all the numbers ...
	    cache_ = nullptr;
    }

#define mAddAttrOut(todocs) \
{ \
    DataPackOutput* attrout = new DataPackOutput(todocs); \
    attrout->setGeometry( todocs ); \
    attrout->setUndefValue( udfval_ ); \
    proc->addOutput( attrout ); \
}

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return nullptr;

    if ( !cache_ )
	mAddAttrOut( tkzs_ )
    else
    {
	const TrcKeyZSampling cachecs = cache_->sampling();
	TrcKeyZSampling todocs( tkzs_ );
	if ( mRg(h).start_.inl() > tkzs_.hsamp_.start_.inl() )
	{
	    todocs.hsamp_.stop_.inl() =
		mRg(h).start_.inl() - tkzs_.hsamp_.step_.inl();
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop_.inl() < tkzs_.hsamp_.stop_.inl() )
	{
	    todocs = tkzs_;
	    todocs.hsamp_.start_.inl() =
		mRg(h).stop_.inl() + tkzs_.hsamp_.step_.inl();
	    mAddAttrOut( todocs )
	}

	const int startinl =
		mMAX(tkzs_.hsamp_.start_.inl(), mRg(h).start_.inl() );
	const int stopinl = mMIN( tkzs_.hsamp_.stop_.inl(), mRg(h).stop_.inl());

	if ( mRg(h).start_.crl() > tkzs_.hsamp_.start_.crl() )
	{
	    todocs = tkzs_;
	    todocs.hsamp_.start_.inl() = startinl;
	    todocs.hsamp_.stop_.inl() = stopinl;
	    todocs.hsamp_.stop_.crl() =
		mRg(h).start_.crl() - tkzs_.hsamp_.step_.crl();
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop_.crl() < tkzs_.hsamp_.stop_.crl() )
	{
	    todocs = tkzs_;
	    todocs.hsamp_.start_.inl() = startinl;
	    todocs.hsamp_.stop_.inl() = stopinl;
	    todocs.hsamp_.start_.crl() =
		mRg(h).stop_.crl() + tkzs_.hsamp_.step_.crl();
	    mAddAttrOut( todocs )
	}

	todocs = tkzs_;
	todocs.hsamp_.start_.inl() = startinl;
	todocs.hsamp_.stop_.inl() = stopinl;
	todocs.hsamp_.start_.crl() =
		mMAX( tkzs_.hsamp_.start_.crl(), mRg(h).start_.crl() );
	todocs.hsamp_.stop_.crl() =
		mMIN( tkzs_.hsamp_.stop_.crl(), mRg(h).stop_.crl() );

        if ( mRg(z).start_ > tkzs_.zsamp_.start_ + mStepEps*tkzs_.zsamp_.step_ )
	{
            todocs.zsamp_.stop_ = mMAX( mRg(z).start_-tkzs_.zsamp_.step_,
                                       todocs.zsamp_.start_ );
	    mAddAttrOut( todocs )
	}

        if ( mRg(z).stop_ < tkzs_.zsamp_.stop_ - mStepEps*tkzs_.zsamp_.step_ )
	{
	    todocs.zsamp_ = tkzs_.zsamp_;
            todocs.zsamp_.start_ = mMIN( mRg(z).stop_+tkzs_.zsamp_.step_,
                                         todocs.zsamp_.stop_ );
	    mAddAttrOut( todocs )
	}
    }

    if ( tkzs_.isFlat() && tkzs_.defaultDir() != TrcKeyZSampling::Z )
    {
	TypeSet<BinID> positions;
	if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
	    for ( int idx=0; idx<tkzs_.nrCrl(); idx++ )
		positions += BinID( tkzs_.hsamp_.start_.inl(),
				    tkzs_.hsamp_.start_.crl() +
					tkzs_.hsamp_.step_.crl()*idx );
	if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
	    for ( int idx=0; idx<tkzs_.nrInl(); idx++ )
		positions += BinID( tkzs_.hsamp_.start_.inl() +
				    tkzs_.hsamp_.step_.inl()*idx,
				    tkzs_.hsamp_.start_.crl() );

	proc->setRdmPaths( positions, positions );
    }

    return proc;
}


class AEMFeatureExtracter : public Executor
{ mODTextTranslationClass(AEMFeatureExtracter);
public:
AEMFeatureExtracter( EngineMan& aem, const BufferStringSet& inputs,
		     const ObjectSet<BinIDValueSet>& bivsets )
    : Executor("Extracting attributes")
{
    const DescSet* attrset =
	aem.procattrset_ ? aem.procattrset_ : aem.inpattrset_;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DescID id = attrset->getID( inputs.get(idx), true );
	if ( id == DescID::undef() )
	    continue;
	SelSpec ss( 0, id );
	ss.setRefFromID( *attrset );
	aem.attrspecs_ += ss;
    }

    ObjectSet<BinIDValueSet>& bvs =
	const_cast<ObjectSet<BinIDValueSet>&>(bivsets);

    proc_ = aem.createLocationOutput( errmsg_, bvs );
}

~AEMFeatureExtracter()		{ delete proc_; }

od_int64 totalNr() const override  { return proc_ ? proc_->totalNr() : -1; }
od_int64 nrDone() const override   { return proc_ ? proc_->nrDone() : 0; }
uiString uiNrDoneText() const override
{
    return proc_ ? proc_->uiNrDoneText() : uiString::emptyString();
}

uiString uiMessage() const override
{
    return !errmsg_.isEmpty()
	? errmsg_
	: (proc_
	   ? proc_->uiMessage()
	   : uiStrings::phrCannotCreate(tr("output")) );
}

int haveError( const uiString& msg )
{
    if ( !msg.isEmpty() ) errmsg_ = msg;
    return -1;
}

int nextStep() override
{
    if ( !proc_ ) return haveError( uiString::emptyString() );

    int rv = proc_->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( proc_->uiMessage() );
}

    uiString			errmsg_;
    Processor*			proc_;
    TypeSet<DescID>		outattribs_;
};


Executor* EngineMan::createFeatureOutput( const BufferStringSet& inputs,
				    const ObjectSet<BinIDValueSet>& bivsets )
{
    return new AEMFeatureExtracter( *this, inputs, bivsets );
}


void EngineMan::computeIntersect2D( ObjectSet<BinIDValueSet>& bivsets ) const
{
    if ( !procattrset_ || !attrspecs_.size() )
	return;

    if ( !procattrset_->is2D() )
	return;

    RefMan<Desc> storeddesc;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
    {
	ConstRefMan<Desc> desc = procattrset_->getDesc( attrspecs_[idx].id() );
	if ( !desc )
	    continue;

	if ( desc->isStored() )
	{
	    storeddesc = desc.getNonConstPtr();
	    break;
	}
	else
	{
	    RefMan<Desc> candidate = desc->getStoredInput();
	    if ( candidate )
	    {
		storeddesc = candidate;
		break;
	    }
	}
    }

    if ( !storeddesc )
	return;

    const ValParam* param = storeddesc->getValParam( StorageProvider::keyStr());
    if ( !param )
	return;

    const MultiID key = param->getMultiID();
    ConstPtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return;

    const Seis2DDataSet dset( *ioobj );
    PosInfo::LineSet2DData linesetgeom;
    for ( int idx=0; idx<dset.nrLines(); idx++ )
    {
	PosInfo::Line2DData& linegeom = linesetgeom.addLine(dset.lineName(idx));
	Pos::GeomID geomid = Survey::GM().getGeomID( dset.lineName(idx) );
	const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid );
	mDynamicCastGet( const Survey::Geometry2D*, geom2d, geometry )
	if ( geom2d )
	    linegeom = geom2d->data();
	if ( linegeom.positions().isEmpty() )
	{
	    linesetgeom.removeLine( dset.lineName(idx) );
	    continue;
	}
    }

    ObjectSet<BinIDValueSet> newbivsets;
    for ( int idx=0; idx<bivsets.size(); idx++ )
    {
	BinIDValueSet* newset = new BinIDValueSet(bivsets[idx]->nrVals(), true);
	ObjectSet<PosInfo::LineSet2DData::IR> resultset;
	linesetgeom.intersect( *bivsets[idx], resultset );

	for ( int idy=0; idy<resultset.size(); idy++)
	    newset->append(*resultset[idy]->posns_);

	newbivsets += newset;
    }
    bivsets = newbivsets;
}


Processor* EngineMan::createLocationOutput( uiString& errmsg,
					    ObjectSet<BinIDValueSet>& bidzvset )
{
    if ( bidzvset.size() == 0 ) return 0;

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0;

    computeIntersect2D(bidzvset);
    ObjectSet<LocationOutput> outputs;
    for ( int idx=0; idx<bidzvset.size(); idx++ )
    {
	BinIDValueSet& bidzvs = *bidzvset[idx];
	LocationOutput* attrout = new LocationOutput( bidzvs );
	outputs += attrout;
    }

    if ( !outputs.size() )
	return 0;

    for ( int idx=0; idx<outputs.size(); idx++ )
	proc->addOutput( outputs[idx] );

    return proc;
}


//TODO TableOutput should later on replace at least Location- and TrcSel-Output
//AEMFeatureExtractor will also be replaced by AEMTableExtractor
class AEMTableExtractor : public Executor
{ mODTextTranslationClass(AEMTableExtractor);
public:
AEMTableExtractor( EngineMan& aem, DataPointSet& datapointset,
		   const Attrib::DescSet& descset, int firstcol )
    : Executor("Extracting attributes")
{
    const int nrinps = datapointset.nrCols();
    for ( int idx=0; idx<nrinps; idx++ )
    {
	FileMultiString fms( datapointset.colDef(idx).ref_ );
	if ( fms.size() < 2 )
	    continue;
	const DescID did( fms.getIValue(1), descset.containsStoredDescOnly() );
	if ( did == DescID::undef() )
	    continue;
	SelSpec ss( 0, did );
	ss.setRefFromID( descset );
	aem.attrspecs_.addIfNew( ss );
    }

    proc_ = aem.getTableOutExecutor( datapointset, errmsg_, firstcol );
}

~AEMTableExtractor()		{ delete proc_; }

od_int64 totalNr() const override  { return proc_ ? proc_->totalNr() : -1; }
od_int64 nrDone() const override   { return proc_ ? proc_->nrDone() : 0; }
uiString uiNrDoneText() const override
{
    return proc_ ? proc_->uiNrDoneText() : uiString::emptyString();
}

uiString uiMessage() const override
{
    return !errmsg_.isEmpty()
	? errmsg_
	: (proc_
	    ? proc_->Task::uiMessage()
	    : uiStrings::phrCannotCreate(uiStrings::sOutput() ));
}

int haveError( const uiString& msg )
{
    if ( !msg.isEmpty() ) errmsg_ = msg;
    return -1;
}

int nextStep() override
{
    if ( !proc_ ) return haveError( uiString::emptyString() );

    int rv = proc_->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( proc_->uiMessage() );
}

    uiString			errmsg_;
    Processor*			proc_;
    TypeSet<DescID>		outattribs_;
};


Executor* EngineMan::getTableExtractor( DataPointSet& datapointset,
					const Attrib::DescSet& descset,
					uiString& errmsg, int firstcol,
					bool needprep )
{
    if ( needprep && !ensureDPSAndADSPrepared( datapointset, descset, errmsg ) )
	return 0;

    setAttribSet( &descset );
    AEMTableExtractor* tabex = new AEMTableExtractor( *this, datapointset,
						      descset, firstcol );
    if ( tabex && !tabex->errmsg_.isEmpty() )
	errmsg = tabex->errmsg_;
    return tabex;
}


Processor* EngineMan::getTableOutExecutor( DataPointSet& datapointset,
					   uiString& errmsg, int firstcol )
{
    if ( !datapointset.size() ) return 0;

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0;

    ObjectSet<BinIDValueSet> bidsets;
    bidsets += &datapointset.bivSet();
    computeIntersect2D( bidsets );
    TableOutput* tableout = new TableOutput( datapointset, firstcol );
    if ( !tableout ) return 0;

    proc->addOutput( tableout );

    return proc;
}


#define mErrRet(s) { errmsg = s; return 0; }

Processor* EngineMan::getProcessor( uiString& errmsg )
{
    if ( procattrset_ )
	{ delete procattrset_; procattrset_ = 0; }

    if ( !inpattrset_ || !attrspecs_.size() )
	mErrRet( tr("No attribute set or input specs") )

    TypeSet<DescID> outattribs;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
	outattribs += attrspecs_[idx].id();

    DescID outid = outattribs[0];

    errmsg = uiString::emptyString();
    bool doeval = false;
    if ( !attrspecs_[0].isNLA() )
    {
	procattrset_ = inpattrset_->optimizeClone( outattribs );
	if ( !procattrset_ ) mErrRet(tr("Attribute set not valid"));

	if ( outattribs.size() > 1 )
	{
	    doeval = true;
	    outid = createEvaluateADS( *procattrset_, outattribs, errmsg);
	}
    }
    else
    {
	DescID nlaid( SelSpec::cNoAttrib() );
	procattrset_ = createNLAADS( nlaid, errmsg );
	if ( !procattrset_ )
	    mErrRet(errmsg)
	outid = nlaid;
    }

    Processor* proc = createProcessor(*procattrset_,
			Survey::GM().getName(geomid_), outid, errmsg);
    setExecutorName( proc );
    if ( !proc )
	mErrRet( errmsg )

    if ( doeval )
    {
	for ( int idx=1; idx<attrspecs_.size(); idx++ )
	    proc->addOutputInterest(idx);
    }

    if ( proc->getProvider() )
	proc->getProvider()->setGeomID( geomid_ );

    return proc;
}


Processor* EngineMan::createTrcSelOutput( uiString& errmsg,
					  const BinIDValueSet& bidvalset,
					  SeisTrcBuf& output, float outval,
					  const Interval<float>* cubezbounds,
					  const TypeSet<BinID>* trueknotspos,
					  const TypeSet<BinID>* snappedpos )
{
    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return nullptr;

    auto* attrout = new TrcSelectionOutput( bidvalset, outval );
    attrout->setOutput( &output );
    if ( cubezbounds )
	attrout->setTrcsBounds( *cubezbounds );

    attrout->setGeomID( geomid_ );

    proc->addOutput( attrout );
    if ( trueknotspos && snappedpos )
	proc->setRdmPaths( *trueknotspos, *snappedpos );

    return proc;
}


Processor* EngineMan::create2DVarZOutput( uiString& errmsg,
					  const IOPar& pars,
					  DataPointSet* datapointset,
					  float outval,
					  const Interval<float>* cubezbounds )
{
    Processor* proc = getProcessor( errmsg );
    if ( !proc )
	return nullptr;

    auto* attrout = new Trc2DVarZStorOutput( geomid_, datapointset, outval );
    attrout->doUsePar( pars );
    if ( cubezbounds )
	attrout->setTrcsBounds( *cubezbounds );

    proc->addOutput( attrout );
    return proc;
}


int EngineMan::getNrOutputsToBeProcessed( const Processor& proc ) const
{
    return proc.outputs_.size();
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }

bool EngineMan::ensureDPSAndADSPrepared( DataPointSet& datapointset,
					 const Attrib::DescSet& descset,
					 uiString& errmsg )
{
    BufferStringSet attrrefs;
    descset.fillInAttribColRefs( attrrefs );

    for ( int idx=0; idx<datapointset.nrCols(); idx++ )
    {
	DataColDef& dcd = datapointset.colDef( idx );

	const char* nmstr = dcd.name_.buf();
	if ( *nmstr == '[' ) // Make sure stored descs are actually in the set
	{
	    int refidx = -1;
	    for ( int ids=0; ids<attrrefs.size(); ids++ )
	    {
		FileMultiString fms( attrrefs.get(ids) );
		if ( fms[0]==nmstr )
		    { refidx = ids; break; }
	    }

	    //TODO : handle multi components stored data
	    DescID descid = DescID::undef();
	    if ( refidx > -1 )
	    {
		const FileMultiString fms( attrrefs.get(refidx) );
		MultiID key;
		key.fromString( fms[1].buf() );
		descid = cCast(DescSet&,descset).getStoredID( key, 0, true );
	    }

	    if ( descid == DescID::undef() )
		mErrRet( tr("Cannot find specified '%1'",
			    " in object management").arg(nmstr));

	    // Put the new DescID in coldef and in the refs
	    BufferString tmpstr;
	    ConstRefMan<Attrib::Desc> desc = descset.getDesc( descid );
	    if ( !desc )
		mErrRet(toUiString("Huh?"));

	    desc->getDefStr( tmpstr );
	    FileMultiString fms( tmpstr ); fms += descid.asInt();
	    attrrefs.get(refidx) = fms;
	    dcd.ref_ = fms;
	}

	if ( dcd.ref_.isEmpty() )
	{
	    int refidx = attrrefs.indexOf( nmstr ); // maybe name == ref
	    if ( refidx == -1 )
	    {
		DescID did = descset.getID( nmstr, true );
		if ( did == DescID::undef() ) // Column is not an attribute
		    continue;

		for ( int idref=0; idref< attrrefs.size(); idref++ )
		{
		    FileMultiString fms( attrrefs.get(idref) );
		    const DescID candidatid( fms.getIValue(1), false );
		    if ( did == candidatid )
			{ refidx = idref; break; }
		}
		if ( refidx < 0 ) // nmstr is not in attrrefs - period.
		    continue;
	    }

	    dcd.ref_ = attrrefs.get( refidx );
	}
    }
    return true;
}

#undef mErrRet

} // namespace Attrib
