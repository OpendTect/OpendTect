/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H.Payraudeau
 Date:          04/2005
________________________________________________________________________

-*/

#include "attribengman.h"

#include "arrayndimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribprovider.h"
#include "attribstorprovider.h"

#include "convmemvalseries.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "linesetposinfo.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "posvecdataset.h"
#include "seis2ddata.h"
#include "seisdatapack.h"
#include "seistrc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "uistrings.h"

#include <string.h>

namespace Attrib
{

EngineMan::EngineMan()
    : attrset_(0)
    , procattrset_(0)
    , nlamodel_(0)
    , dpm_(DPM(DataPackMgr::SeisID()))
    , cache_(0)
    , udfval_(mUdf(float))
    , curattridx_(0)
{
}


EngineMan::~EngineMan()
{
    delete procattrset_;
    delete attrset_;
    delete nlamodel_;
    unRefAndZeroPtr( cache_ );
}


bool EngineMan::is2D() const
{
    return attrset_ ? attrset_->is2D() : subsel_.is2D();
}


uiRetVal EngineMan::getPossibleSubSel( DescSet& attrset, const DescID& outid,
					FullSubSel& fss, GeomID gid )
{
    fss = attrset.is2D() ? FullSubSel(gid) : FullSubSel();
    TypeSet<DescID> desiredids( 1, outid );

    uiRetVal uirv;
    DescID evalid = createEvaluateADS( attrset, desiredids, uirv );
    PtrMan<Processor> proc = createProcessor( attrset, evalid, uirv, gid );
    if ( !proc )
	return uirv;

    proc->computeAndSetRefZStepAndZ0();
    proc->setDesiredSubSel( fss );
    fss = proc->getProvider()->possibleSubSel();
    return uirv;
}


Processor* EngineMan::createProcessor( const DescSet& attrset,
				       const DescID& outid,
				       uiRetVal& uirv, GeomID gid )
{
    Desc* targetdesc = const_cast<Desc*>(attrset.getDesc(outid));
    if ( !targetdesc )
	return 0;

    targetdesc->updateParams();
    Processor* processor = new Processor( *targetdesc, uirv, gid );
    if ( !processor->isOK() )
	{ delete processor; return 0; }

    processor->addOutputInterest( targetdesc->selectedOutput() );
    return processor;
}


void EngineMan::setGeomID( GeomID gid )
{
    if ( !gid.isValid() )
	return;

    if ( attrset_ && attrset_->is2D() != gid.is2D() )
	{ pErrMsg("2D/3D mismatch"); }

    if ( subsel_.geomID(0) != gid )
	subsel_ = FullSubSel( gid );
}


Processor* EngineMan::usePar( const IOPar& iopar, DescSet& attrset,
			      uiRetVal& uirv, int outidx, GeomID gid )
{
    PtrMan<IOPar> outputpar =
		iopar.subselect( IOPar::compKey(sKey::Output(),outidx) );
    if ( !outputpar )
	return 0;

    TypeSet<DescID> ids;
    int attribidx = 0;
    while ( true )
    {
	BufferString attribidstr =
		    IOPar::compKey( sKey::Attributes(), attribidx );
	int attribid;
	if ( !outputpar->get(attribidstr,attribid) )
	    break;

	ids += DescID( attribid );
	attribidx++;
    }

    DescID evalid = createEvaluateADS( attrset, ids, uirv );
    Processor* proc = createProcessor( attrset, evalid, uirv, gid );
    if ( !proc )
	return proc;

    for ( int idx=1; idx<ids.size(); idx++ )
	proc->addOutputInterest( idx );

    if ( attrset.is2D() )
	subsel_ = FullSubSel( gid );
    else
	subsel_.setToAll( false );

    PtrMan<IOPar> subselpar =
	iopar.subselect( IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( subselpar )
       subsel_.usePar( *subselpar );

    SeisTrcStorOutput* storeoutp = createOutput( iopar, uirv, outidx );
    if ( !storeoutp )
	return nullptr;

    bool exttrctosi = false;
    const BufferString basekey = IOPar::compKey( sKey::Output(), outidx );
    const BufferString extkey =
			IOPar::compKey( basekey, SeisTrc::sKeyExtTrcToSI() );
    if ( iopar.getYN(extkey,exttrctosi) )
	storeoutp->setTrcGrow( exttrctosi );

    BufferStringSet outnms;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const StringPair sp( attrset.getDesc(ids[idx])->userRef() );
	outnms.add( !sp.second().isEmpty() ? sp.second() : sp.first() );
    }

    storeoutp->setOutpNames( outnms );
    proc->addOutput( storeoutp );
    return proc;
}


void EngineMan::setExecutorName( Executor* ex )
{
    if ( !ex ) return;

    BufferString usernm( getCurUserRef() );
    if ( usernm.isEmpty() || !attrset_ )
	return;

    if ( curattridx_ < 0 || curattridx_ >= attrspecs_.size() )
	ex->setName( "Processing attribute" );

    SelSpec& ss = attrspecs_[curattridx_];
    BufferString nm( "Calculating " );
    if ( ss.isNLA() && nlamodel_ )
    {
	nm = "Applying ";
	nm += nlamodel_->nlaType(true);
	nm += ": calculating";
	if ( DBKey::isValidString(usernm) )
	    usernm = DBKey(usernm).name();
    }
    else
    {
	const Desc* desc = attrset_->getDesc( ss.id() );
	if ( desc && desc->isStored() )
	    nm = "Reading from";
    }

    nm += " \"";
    nm += usernm;
    nm += "\"";

    ex->setName( nm );
}


SeisTrcStorOutput* EngineMan::createOutput( const IOPar& pars, uiRetVal& uirv,
					    int outidx )
{
    const FixedString typestr =
		pars.find( IOPar::compKey(sKey::Output(),sKey::Type()) );
    if ( typestr==sKey::Cube() )
    {
	SeisTrcStorOutput* outp = new SeisTrcStorOutput( subsel_ );
	const bool res = outp->doUsePar( pars, outidx );
	if ( !res )
	    { uirv = outp->errMsg(); delete outp; outp = nullptr; }
	return outp;
    }

    return nullptr;
}


void EngineMan::setNLAModel( const NLAModel* m )
{
    delete nlamodel_;
    nlamodel_ = m ? m->clone() : 0;
}


void EngineMan::setAttribSet( const DescSet* ads )
{
    delete attrset_;
    attrset_ = ads ? new DescSet( *ads ) : 0;
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
	if ( !attrset_ ) return "";
	ss.setRefFromID( *attrset_ );
    }
    return attrspecs_[idx].userRef();
}


RefMan<RegularSeisDataPack>
    EngineMan::getDataPackOutput( const Processor& proc )
{
    RefMan<RegularSeisDataPack> output = 0;
    if ( proc.outputs_.size()==1 && !cache_ )
    {
	output = const_cast<RegularSeisDataPack*>(
			proc.outputs_[0]->getDataPack() );
	if ( !output || !output->sampling().isDefined() )
	    return 0;

	for ( int idx=0; idx<attrspecs_.size(); idx++ )
	    output->setComponentName( attrspecs_[idx].userRef(), idx );

	output->setZDomain(
		ZDomain::Info(ZDomain::Def::get(attrspecs_[0].zDomainKey())) );
	output->setName( attrspecs_[0].userRef() );
	return output;
    }

    ObjectSet<const RegularSeisDataPack> packset;
    for ( int idx=0; idx<proc.outputs_.size(); idx++ )
    {
	ConstRefMan<RegularSeisDataPack> dp =
		proc.outputs_[idx] ? proc.outputs_[idx]->getDataPack() : 0;
	if ( !dp || !dp->sampling().isDefined() )
	    continue;

	dpm_.add( const_cast<RegularSeisDataPack*>( dp.ptr()) );

	dp->ref();
	packset += dp;
    }

    if ( cache_ )
    {
	packset += cache_;
	cache_->ref();
    }

    if ( !packset.isEmpty() )
	output =
	   const_cast<RegularSeisDataPack*>( getDataPackOutput(packset).ptr() );

    deepUnRef( packset );

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
    worktkzs_.limitTo( in.sampling(), true );
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

od_int64 nrIterations() const		{ return totalnr_; }

bool doPrepare( int nrthreads )
{
    if ( in_.isEmpty() || out_.isEmpty() )
	return false;

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
	inptr_[idc] = rCast( ConstCompPtr, in_.data(incomp_[idc]).getData() );
	mDynamicCastGet( const ConvMemValueSeries<float>*, instorage,
			 in_.data(incomp_[idc]).getStorage() );
	if ( instorage )
	{
	    inptr_[idc] = rCast( ConstCompPtr, instorage->storArr() );
	    samplebytes_ = in_.getDataDesc().nrBytes();
	}

	outptr_[idc] = rCast( CompPtr, out_.data(idc).getData() );
	mDynamicCastGet( ConvMemValueSeries<float>*, outstorage,
			 out_.data(idc).getStorage() );
	if ( outstorage )
	    outptr_[idc] = rCast( CompPtr, outstorage->storArr() );

	if ( !inptr_[idc] || !outptr_[idc] )
	    return true;

	const float start = worktkzs_.zsamp_.start;
	inptr_[idc] += samplebytes_ * in_.getZRange().nearestIndex( start );
	outptr_[idc] += samplebytes_ * out_.getZRange().nearestIndex( start );
    }

    intracebytes_ = samplebytes_ * in_.sampling().size(OD::ZSlice);
    inlinebytes_ = intracebytes_ * in_.sampling().size(OD::CrosslineSlice);

    outtracebytes_ = samplebytes_ * out_.sampling().size(OD::ZSlice);
    outlinebytes_ = outtracebytes_ * out_.sampling().size(OD::CrosslineSlice);

    bytestocopy_ = samplebytes_ * worktkzs_.nrZ();

    domemcopy_ = true;
    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const auto inhs = in_.sampling().hsamp_;
    const auto ouths = out_.sampling().hsamp_;
    const auto workhs = worktkzs_.hsamp_;

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

    if ( !domemcopy || !mIsEqual(inzsamp.step, outzsamp.step, mDefEpsF) )
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

    const int nrtrcs = workhs.nrTrcs();
    int inlidx = mCast( int, start/nrtrcs );
    int crlidx = mCast( int, start%nrtrcs );

    const int halfinlstep = inhs.step_.lineNr() / 2;
    BinID bid = workhs.atIndex( inlidx, 0 );
    int outinlidx = ouths.lineIdx( bid.inl() );
    int shiftedtogetnearestinl = bid.lineNr() + halfinlstep;
    int ininlidx = inhs.lineIdx( shiftedtogetnearestinl );

    int* incrllut = new int[ nrtrcs ];
    int* outcrllut = new int[ nrtrcs ];
    const int halfcrlstep = inhs.step_.trcNr() / 2;
    for ( int cidx=0; cidx<nrtrcs; cidx++ )
    {
	bid = workhs.atIndex( 0, cidx );
	outcrllut[cidx] = ouths.trcIdx( bid.crl() );
	const int shiftedtogetnearestcrl = bid.trcNr() + halfcrlstep;
	incrllut[cidx] = inhs.trcIdx( shiftedtogetnearestcrl );
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
	    bid = workhs.atIndex( ++inlidx, 0 );
	    outinlidx = ouths.lineIdx( bid.inl() );
	    shiftedtogetnearestinl = bid.lineNr() + halfinlstep;
	    ininlidx = inhs.lineIdx( shiftedtogetnearestinl );
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
    if ( packset.isEmpty() ) return 0;
    const char* category = VolumeDataPack::categoryStr(
				!subsel_.isZSlice(), subsel_.is2D() );
    RegularSeisDataPack* output =
	new RegularSeisDataPack( category, &packset[0]->getDataDesc() );
    DPM(DataPackMgr::SeisID()).add( output );
    if ( packset[0]->getScaler() )
	output->setScaler( *packset[0]->getScaler() );

    if ( cache_ && cache_->sampling().zsamp_.step != subsel_.zSubSel().zStep() )
    {
	TrcKeyZSampling cswithcachestep( subsel_ );
	cswithcachestep.zsamp_.step = cache_->sampling().zsamp_.step;
	output->setSampling( cswithcachestep );
    }
    else
    {
	// For running dimensions, steps of TrcKeyZSampling (Inl, Crl and Z)
	// should be same as that of datapack (packset) that is preloaded.
	TrcKeyZSampling availabletkzs = packset[0]->sampling();
	for ( int idx=1; idx<packset.size(); idx++ )
	    availabletkzs.include( packset[idx]->sampling() );

	TrcKeyZSampling outputtkzs = TrcKeyZSampling( subsel_ );
	outputtkzs.adjustTo( availabletkzs, true );
	output->setSampling( outputtkzs );
    }

    for ( int idx=0; idx<attrspecs_.size(); idx++ )
    {
	const char* compnm = attrspecs_[idx].userRef();
	if ( packset[0]->getComponentIdx(compnm,idx) >= 0 )
	    output->addComponent( compnm, true );
    }

    output->setZDomain(
	    ZDomain::Info(ZDomain::Def::get(attrspecs_[0].zDomainKey())) );
    output->setName( attrspecs_[0].userRef() );

    for ( int iset=0; iset<packset.size(); iset++ )
    {
	const RegularSeisDataPack& regsdp = *packset[iset];
	DataPackCopier copier( regsdp, *output );
	copier.execute();
    }

    return output;
}


void EngineMan::setAttribSpecs( const SelSpecList& specs )
{ attrspecs_ = specs; }


void EngineMan::setAttribSpec( const SelSpec& spec )
{
    attrspecs_.erase();
    attrspecs_ += spec;
}


void EngineMan::setSubSel( const FullSubSel& fss )
{
    if ( attrset_ && fss.is2D() != attrset_->is2D() )
	{ pErrMsg("2D/3D mismatch"); return; }
    subsel_ = fss;
}


DescSet* EngineMan::createNLAADS( DescID& nladescid, uiRetVal& uirv,
				  const DescSet* addtoset )
{
    if ( !nlamodel_ )
	{ uirv = mINTERNAL("No NLA Model"); return 0; }

    if ( attrspecs_.isEmpty() )
	return nullptr;
    DescSet* descset = addtoset ? new DescSet( *addtoset )
				: new DescSet( attrspecs_[0].is2D() );

    if ( !addtoset )
    {
	uirv = descset->usePar( nlamodel_->pars() );
	if ( !uirv.isOK() )
	    { delete descset; return nullptr; }
    }

    BufferString s;
    nlamodel_->dump(s);
    BufferString defstr( nlamodel_->nlaType(true) );
    defstr += " specification=\""; defstr += s; defstr += "\"";

    addNLADesc( defstr, nladescid, *descset, attrspecs_[0].id().getI(),
		nlamodel_, uirv );

    DescSet* cleanset = descset->optimizeClone( nladescid );
    delete descset;
    return cleanset;
}


void EngineMan::addNLADesc( const char* specstr, DescID& nladescid,
			    DescSet& descset, int outputnr,
			    const NLAModel* nlamdl, uiRetVal& uirv )
{
    RefMan<Desc> desc = PF().createDescCopy( "NN" );
    desc->setDescSet( &descset );

    if ( !desc->parseDefStr(specstr) )
    {
	uirv = tr("Invalid definition string for NLA model:\n%1")
		    .arg( specstr );
	return;
    }
    desc->setIsHidden( true );

    // Need to make a Provider because the inputs and outputs may
    // not be known otherwise
    uirv = Provider::prepare( *desc );
    if ( !uirv.isOK() )
	return;

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
		if ( DBKey::isValidString(inpname) )
		    ioobj = DBKey(inpname).getIOObj();
		else
		{
		    BufferString rawnmbufstr;
	    //because constructor has strange behaviour with embeded strings
		    rawnmbufstr += inpname;
		    rawnmbufstr.unEmbed( '[', ']' );
		    if ( rawnmbufstr.buf() && inpname &&
			 strcmp( rawnmbufstr.buf(), inpname ) )
		    {
			const char* tgname = descset.is2D() ? "2D Seismic Data"
							    : "Seismic Data";
			ioobj = DBM().getByName( rawnmbufstr.buf(), tgname );
		    }
		}
		if ( ioobj )
		{
		    Desc* stordesc =
			PF().createDescCopy( StorageProvider::attribName() );
		    stordesc->setDescSet( &descset );
		    ValParam* idpar =
			stordesc->getValParam( StorageProvider::keyStr() );
		    idpar->setValue( ioobj->key().toString() );
		    stordesc->setUserRef( ioobj->name() );
		    descid = descset.addDesc( stordesc );
		    if ( !descid.isValid() )
		    {
			uirv = tr("NLA input '%1' cannot be found in "
				    "the provided set.").arg( inpname );
			return;
		    }
		}
	    }
	}

	desc->setInput( idx, descset.getDesc(descid) );
    }

    if ( outputnr > desc->nrOutputs() )
    {
	uirv = tr("Output %1 not present.").arg( toString(outputnr) );
	return;
    }

    const NLADesign& nlades = nlamdl->design();
    desc->setUserRef( *nlades.outputs[outputnr] );
    desc->selectOutput( outputnr );

    nladescid = descset.addDesc( desc );
    if ( !nladescid.isValid() )
	uirv = tr("Error setting learning attribute");
}


DescID EngineMan::createEvaluateADS( DescSet& descset,
				     const TypeSet<DescID>& outids,
				     uiRetVal& uirv )
{
    if ( outids.isEmpty() )
	return DescID();
    if ( outids.size() == 1 ) return outids[0];

    Desc* desc = PF().createDescCopy( "Evaluate" );
    if ( !desc )
	return DescID();

    desc->setDescSet( &descset );
    desc->setNrOutputs( Seis::UnknownData, outids.size() );

    desc->setIsHidden( true );

    const int nrinputs = outids.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	desc->addInput( InputSpec("Data",true) );
	Desc* inpdesc = descset.getDesc( outids[idx] );
	if ( !inpdesc ) continue;

	desc->setInput( idx, inpdesc );
    }

    desc->setUserRef( "evaluate attributes" );
    desc->selectOutput(0);

    DescID evaldescid = descset.addDesc( desc );
    if ( !evaldescid.isValid() )
    {
	uirv = tr("Cannot set evaluation attribute");
	desc->unRef();
    }

    return evaldescid;
}


#define mStepEps 1e-3


Processor* EngineMan::createScreenOutput2D( uiRetVal& uirv,
					    Data2DHolder& output )
{
    Processor* proc = getProcessor( uirv );
    if ( !proc )
	return 0;

    TwoDOutput* attrout = new TwoDOutput( subsel_.trcNrRange(),
					  subsel_.zRange(), subsel_.geomID(0) );
    attrout->setOutput( output );
    proc->addOutput( attrout );

    return proc;
}

#define mRg(dir) (cachecs.dir##samp_)

Processor* EngineMan::createDataPackOutput( uiRetVal& uirv,
					    const RegularSeisDataPack* prev )
{
    unRefAndZeroPtr( cache_ );
    const TrcKeyZSampling tkzs( subsel_ );

    if ( prev )
    {
	cache_ = prev;
	cache_->ref();
	const TrcKeyZSampling cachecs = cache_->sampling();
	if ( (mRg(h).start_.inl() - tkzs.hsamp_.start_.inl()) %
		tkzs.hsamp_.step_.inl()
	  || (mRg(h).start_.crl() - tkzs.hsamp_.start_.crl()) %
		tkzs.hsamp_.step_.crl()
	  || mRg(h).start_.inl() > tkzs.hsamp_.stop_.inl()
	  || mRg(h).stop_.inl() < tkzs.hsamp_.start_.inl()
	  || mRg(h).start_.crl() > tkzs.hsamp_.stop_.crl()
	  || mRg(h).stop_.crl() < tkzs.hsamp_.start_.crl()
	  || mRg(z).start > tkzs.zsamp_.stop + mStepEps*tkzs.zsamp_.step
	  || mRg(z).stop < tkzs.zsamp_.start - mStepEps*tkzs.zsamp_.step )
	    // No overlap, gotta crunch all the numbers ...
	{
	    unRefAndZeroPtr( cache_ );
	}
    }

#define mAddAttrOut(todocs) \
{ \
    DataPackOutput* attrout = new DataPackOutput(todocs); \
    attrout->setGeometry( todocs ); \
    attrout->setUndefValue( udfval_ ); \
    proc->addOutput( attrout ); \
}

    Processor* proc = getProcessor( uirv );
    if ( !proc )
	return 0;

    if ( !cache_ )
	mAddAttrOut( tkzs )
    else
    {
	const TrcKeyZSampling cachecs = cache_->sampling();
	TrcKeyZSampling todocs( tkzs );
	if ( mRg(h).start_.inl() > tkzs.hsamp_.start_.inl() )
	{
	    todocs.hsamp_.stop_.inl() =
		mRg(h).start_.inl() - tkzs.hsamp_.step_.inl();
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop_.inl() < tkzs.hsamp_.stop_.inl() )
	{
	    todocs = tkzs;
	    todocs.hsamp_.start_.inl() =
		mRg(h).stop_.inl() + tkzs.hsamp_.step_.inl();
	    mAddAttrOut( todocs )
	}

	const int startinl =
		mMAX(tkzs.hsamp_.start_.inl(), mRg(h).start_.inl() );
	const int stopinl = mMIN( tkzs.hsamp_.stop_.inl(), mRg(h).stop_.inl());

	if ( mRg(h).start_.crl() > tkzs.hsamp_.start_.crl() )
	{
	    todocs = tkzs;
	    todocs.hsamp_.start_.inl() = startinl;
	    todocs.hsamp_.stop_.inl() = stopinl;
	    todocs.hsamp_.stop_.crl() =
		mRg(h).start_.crl() - tkzs.hsamp_.step_.crl();
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop_.crl() < tkzs.hsamp_.stop_.crl() )
	{
	    todocs = tkzs;
	    todocs.hsamp_.start_.inl() = startinl;
	    todocs.hsamp_.stop_.inl() = stopinl;
	    todocs.hsamp_.start_.crl() =
		mRg(h).stop_.crl() + tkzs.hsamp_.step_.crl();
	    mAddAttrOut( todocs )
	}

	todocs = tkzs;
	todocs.hsamp_.start_.inl() = startinl;
	todocs.hsamp_.stop_.inl() = stopinl;
	todocs.hsamp_.start_.crl() =
		mMAX( tkzs.hsamp_.start_.crl(), mRg(h).start_.crl() );
	todocs.hsamp_.stop_.crl() =
		mMIN( tkzs.hsamp_.stop_.crl(), mRg(h).stop_.crl() );

	if ( mRg(z).start > tkzs.zsamp_.start + mStepEps*tkzs.zsamp_.step )
	{
	    todocs.zsamp_.stop = mMAX( mRg(z).start-tkzs.zsamp_.step,
				       todocs.zsamp_.start );
	    mAddAttrOut( todocs )
	}

	if ( mRg(z).stop < tkzs.zsamp_.stop - mStepEps*tkzs.zsamp_.step )
	{
	    todocs.zsamp_ = tkzs.zsamp_;
	    todocs.zsamp_.start = mMIN( mRg(z).stop+tkzs.zsamp_.step,
					todocs.zsamp_.stop );
	    mAddAttrOut( todocs )
	}
    }

    if ( tkzs.isFlat() && tkzs.defaultDir() != OD::ZSlice )
    {
	TypeSet<BinID> positions;
	if ( tkzs.defaultDir() == OD::InlineSlice )
	    for ( int idx=0; idx<tkzs.nrCrl(); idx++ )
		positions += BinID( tkzs.hsamp_.start_.inl(),
				    tkzs.hsamp_.start_.crl() +
					tkzs.hsamp_.step_.crl()*idx );
	if ( tkzs.defaultDir() == OD::CrosslineSlice )
	    for ( int idx=0; idx<tkzs.nrInl(); idx++ )
		positions += BinID( tkzs.hsamp_.start_.inl() +
				    tkzs.hsamp_.step_.inl()*idx,
				    tkzs.hsamp_.start_.crl() );

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
	aem.procattrset_ ? aem.procattrset_ : aem.attrset_;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DescID id = attrset->getID( inputs.get(idx), true );
	if ( !id.isValid() )
	    continue;
	SelSpec ss( 0, id );
	ss.setRefFromID( *attrset );
	aem.attrspecs_ += ss;
    }

    ObjectSet<BinIDValueSet>& bvs =
	const_cast<ObjectSet<BinIDValueSet>&>(bivsets);

    proc_ = aem.createLocationOutput( uirv_, bvs );
}

~AEMFeatureExtracter()		{ delete proc_; }

od_int64 totalNr() const	{ return proc_ ? proc_->totalNr() : -1; }
od_int64 nrDone() const		{ return proc_ ? proc_->nrDone() : 0; }
uiString nrDoneText() const
{
    return proc_ ? proc_->nrDoneText() : uiString::empty();
}

uiString message() const
{
    return !uirv_.isOK()
	? uiString(uirv_)
	: (proc_ ? proc_->message()
	   : uiStrings::phrCannotCreate(uiStrings::sOutput().toLower()) );
}

int errorReturn( const uiString& msg )
{
    if ( !msg.isEmpty() )
	uirv_ = msg;
    return -1;
}

int nextStep()
{
    if ( !proc_ )
	return errorReturn( mINTERNAL("General Error") );

    int rv = proc_->doStep();
    if ( rv >= 0 ) return rv;
    return errorReturn( proc_->message() );
}

    uiRetVal			uirv_;
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

    Desc* storeddesc = 0;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
    {
	const Desc* desc = procattrset_->getDesc( attrspecs_[idx].id() );
	if ( !desc ) continue;
	if ( desc->isStored() )
	{
	    storeddesc = const_cast<Desc*>(desc);
	    break;
	}
	else
	{
	    Desc* candidate = desc->getStoredInput();
	    if ( candidate )
	    {
		storeddesc = candidate;
		break;
	    }
	}
    }

    if ( !storeddesc )
	return;

    const StringPair storkey( storeddesc->getValParam(
			      StorageProvider::keyStr())->getStringValue(0) );
    const DBKey key( storkey.first() );
    PtrMan<IOObj> ioobj = key.getIOObj();
    if ( !ioobj ) return;

    const Seis2DDataSet dset( *ioobj );
    PosInfo::LineSet2DData linesetgeom;
    for ( int idx=0; idx<dset.nrLines(); idx++ )
    {
	const BufferString lnm = dset.lineName( idx );
	const auto& geom2d = SurvGeom::get2D( lnm  );
	if ( !geom2d.isEmpty() )
	    linesetgeom.addLine( lnm ) = geom2d.data();
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


Processor* EngineMan::createLocationOutput( uiRetVal& uirv,
					    ObjectSet<BinIDValueSet>& bidzvset )
{
    if ( bidzvset.size() == 0 ) return 0;

    Processor* proc = getProcessor( uirv );
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
	const DescID did( fms.getIValue(1) );
	if ( !did.isValid() )
	    continue;
	SelSpec ss( 0, did );
	ss.setRefFromID( descset );
	aem.attrspecs_.addIfNew( ss );
    }

    proc_ = aem.getTableOutExecutor( datapointset, uirv_, firstcol );
}

~AEMTableExtractor()		{ delete proc_; }

od_int64 totalNr() const	{ return proc_ ? proc_->totalNr() : -1; }
od_int64 nrDone() const		{ return proc_ ? proc_->nrDone() : 0; }
uiString nrDoneText() const
{
    return proc_ ? proc_->nrDoneText() : uiString::empty();
}

uiString message() const
{
    return !uirv_.isEmpty()
	? uiString(uirv_)
	: (proc_
	    ? proc_->Task::message()
	    : uiStrings::phrCannotCreate(uiStrings::sOutput() ));
}

int errorReturn( const uiString& msg )
{
    if ( !msg.isEmpty() )
	uirv_ = msg;
    return -1;
}

int nextStep()
{
    if ( !proc_ )
	return errorReturn( uiString::empty() );

    int rv = proc_->doStep();
    if ( rv >= 0 ) return rv;
    return errorReturn( proc_->message() );
}

    uiRetVal			uirv_;
    Processor*			proc_;
    TypeSet<DescID>		outattribs_;
};


Executor* EngineMan::getTableExtractor( DataPointSet& datapointset,
					const Attrib::DescSet& descset,
					uiRetVal& uirv, int firstcol,
					bool needprep )
{
    if ( needprep && !ensureDPSAndADSPrepared( datapointset, descset, uirv ) )
	return 0;

    setAttribSet( &descset );
    AEMTableExtractor* tabex = new AEMTableExtractor( *this, datapointset,
						      descset, firstcol );
    if ( tabex && !tabex->uirv_.isEmpty() )
	uirv = tabex->uirv_;
    return tabex;
}


Processor* EngineMan::getTableOutExecutor( DataPointSet& datapointset,
					   uiRetVal& uirv, int firstcol )
{
    if ( !datapointset.size() ) return 0;

    Processor* proc = getProcessor( uirv );
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


#define mErrRet(s) { uirv = s; return 0; }

Processor* EngineMan::getProcessor( uiRetVal& uirv )
{
    if ( procattrset_ )
	{ delete procattrset_; procattrset_ = 0; }

    if ( !attrset_ || !attrspecs_.size() )
	mErrRet( tr("No attribute set or input specs") )

    TypeSet<DescID> outattribs;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
	outattribs += attrspecs_[idx].id();

    DescID outid = outattribs[0];

    uirv.setOK();
    bool doeval = false;
    if ( !attrspecs_[0].isNLA() )
    {
	procattrset_ = attrset_->optimizeClone( outattribs );
	if ( !procattrset_ )
	    mErrRet(tr("Attribute set not valid"));

	if ( outattribs.size() > 1 )
	{
	    doeval = true;
	    outid = createEvaluateADS( *procattrset_, outattribs, uirv );
	}
    }
    else
    {
	DescID nlaid( SelSpec::cNoAttribID() );
	procattrset_ = createNLAADS( nlaid, uirv );
	if ( !procattrset_ )
	    mErrRet(uirv)
	outid = nlaid;
    }

    Processor* proc = createProcessor( *procattrset_, outid, uirv, geomID() );
    setExecutorName( proc );
    if ( !proc )
	mErrRet( uirv )

    if ( doeval )
    {
	for ( int idx=1; idx<attrspecs_.size(); idx++ )
	    proc->addOutputInterest(idx);
    }

    return proc;
}


Processor* EngineMan::createTrcSelOutput( uiRetVal& uirv,
					  const BinIDValueSet& bidvalset,
					  SeisTrcBuf& output, float outval,
					  const Interval<float>* cubezbounds,
					  const TypeSet<BinID>* trueknotspos,
					  const TypeSet<BinID>* snappedpos )
{
    Processor* proc = getProcessor( uirv );
    if ( !proc )
	return 0;

    TrcSelectionOutput* attrout = new TrcSelectionOutput( bidvalset, outval );
    attrout->setOutput( &output );
    if ( cubezbounds )
	attrout->setTrcsBounds( *cubezbounds );
    attrout->setGeomID( geomID() );

    proc->addOutput( attrout );
    if ( trueknotspos && snappedpos )
	proc->setRdmPaths( *trueknotspos, *snappedpos );

    return proc;
}


Processor* EngineMan::create2DVarZOutput( uiRetVal& uirv, const IOPar& pars,
					  DataPointSet* datapointset,
					  float outval,
					  Interval<float>* cubezbounds )
{
    Processor* proc = getProcessor( uirv );
    if ( !proc )
	return nullptr;

    Trc2DVarZStorOutput* attrout = new Trc2DVarZStorOutput( geomID(),
							datapointset, outval );
    attrout->doUsePar( pars, 0 );
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
#define mErrRet(s) { uirv = s; return false; }

bool EngineMan::ensureDPSAndADSPrepared( DataPointSet& datapointset,
					 const Attrib::DescSet& descset,
					 uiRetVal& uirv )
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
	    DescID descid;
	    if ( refidx > -1 )
	    {
		FileMultiString fms( attrrefs.get(refidx) );
		descid = const_cast<DescSet&>(descset).
				    getStoredID( DBKey(fms[1]), 0, true );
	    }
	    if ( !descid.isValid() )
		mErrRet( tr("Cannot find specified '%1'",
			    " in object management").arg(nmstr));

	    // Put the new DescID in coldef and in the refs
	    BufferString tmpstr;
	    const Attrib::Desc* desc = descset.getDesc( descid );
	    if ( !desc ) mErrRet(toUiString("Huh?"));
	    desc->getDefStr( tmpstr );
	    FileMultiString fms( tmpstr ); fms += descid.getI();
	    attrrefs.get(refidx) = fms;
	    dcd.ref_ = fms;
	}

	if ( dcd.ref_.isEmpty() )
	{
	    int refidx = attrrefs.indexOf( nmstr ); // maybe name == ref
	    if ( refidx == -1 )
	    {
		DescID did = descset.getID( nmstr, true );
		if ( !did.isValid() ) // Column is not an attribute
		    continue;

		for ( int idref=0; idref< attrrefs.size(); idref++ )
		{
		    FileMultiString fms( attrrefs.get(idref) );
		    const DescID candidatid( fms.getIValue(1) );
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
