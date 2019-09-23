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

#define mZRelEps 0.001f


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
    return attrset_ ? attrset_->is2D() : reqss_.is2D();
}


uiRetVal EngineMan::getPossibleSubSel( DescSet& attrset, const DescID& outid,
					FullSubSel& fss, GeomID gid )
{
    fss = attrset.is2D() ? FullSubSel(gid) : FullSubSel();

    uiRetVal uirv;
    DescID evalid = createEvaluateADS( attrset, TypeSet<DescID>(outid), uirv );
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

    if ( reqss_.geomID(0) != gid )
	reqss_ = FullSubSel( gid );
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
	reqss_ = FullSubSel( gid );
    else
	reqss_.setToAll( false );

    PtrMan<IOPar> subselpar =
	iopar.subselect( IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( subselpar )
       reqss_.usePar( *subselpar );

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
	SeisTrcStorOutput* outp = new SeisTrcStorOutput( reqss_ );
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
	if ( !output )
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
	if ( !dp )
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
    worksubsel_ = out.subSel().duplicate();
    worksubsel_->limitTo( in.subSel() );
    totalnr_ = worksubsel_->horSubSel().totalSize();

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
    delete worksubsel_;
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

	const float start = worksubsel_->zRange().start;
	inptr_[idc] += samplebytes_ * in_.zRange().nearestIndex( start );
	outptr_[idc] += samplebytes_ * out_.zRange().nearestIndex( start );
    }

    intracebytes_ = samplebytes_ * in_.subSel().nrZ();
    inlinebytes_ = intracebytes_ * in_.horSubSel().trcNrSize();
    outtracebytes_ = samplebytes_ * out_.subSel().nrZ();
    outlinebytes_ = outtracebytes_ * out_.horSubSel().trcNrSize();
    bytestocopy_ = samplebytes_ * worksubsel_->nrZ();
    domemcopy_ = true;
    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const auto& inhss = in_.horSubSel();
    const auto& outhss = out_.horSubSel();
    const auto& workhss = worksubsel_->horSubSel();
    const bool is2d = workhss.is2D();

    const auto inzrg = in_.zRange();
    const auto outzrg = out_.zRange();
    const auto workzrg = worksubsel_->zRange();

    const auto nrcomps = out_.nrComponents();

    int nrz = worksubsel_->nrZ();
    int* inzidxs = nullptr;
    int* outzidxs = nullptr;

    if ( !domemcopy_ || !mIsEqual(inzrg.step, outzrg.step, mDefEpsF) )
    {
	inzidxs = new int[ nrz ];
	outzidxs = new int[ nrz ];

	const int nrinpzrg = inzrg.nrSteps()+1;
	const int nroutzrg = outzrg.nrSteps()+1;
	for ( int idz=0; idz<nrz; idz++ )
	{
	    const float zval = workzrg.atIndex( idz );
	    inzidxs[idz] = inzrg.nearestIndex( zval );
	    outzidxs[idz] = outzrg.nearestIndex( zval );

	    if ( inzidxs[idz]>=nrinpzrg || outzidxs[idz]>=nroutzrg )
		{ nrz = idz; break; }
	}

	for ( int idz=nrz-1; domemcopy_ && idz>=0; idz-- )
	{
	    // Need relative offsets in case of domemcopy_
	    inzidxs[idz] =  idz ? samplebytes_ * (inzidxs[idz]-inzidxs[idz-1])
				: 0;
	    outzidxs[idz] = idz ? samplebytes_*(outzidxs[idz]-outzidxs[idz-1])
				: 0;
	}
    }

    const auto& worktrcnrss = workhss.trcNrSubSel();
    const auto& intrcnrss = inhss.trcNrSubSel();
    const auto& outtrcnrss = outhss.trcNrSubSel();

    const int worktrcnrsz = worktrcnrss.size();
    int* intrcnridxs = new int[ worktrcnrsz ];
    int* outtrcnridxs = new int[ worktrcnrsz ];
    for ( int tidx=0; tidx<worktrcnrsz; tidx++ )
    {
	const auto tnr = worktrcnrss.pos4Idx( tidx );
	outtrcnridxs[tidx] = outtrcnrss.idx4Pos( tnr );
	intrcnridxs[tidx] = intrcnrss.idx4Pos( tnr );
    }

    for ( auto gidx=start; gidx<=stop; gidx++ )
    {
	const auto lineidx = start / worktrcnrsz;
	if ( is2d && lineidx > 1 )
	    { pErrMsg("Unexpected next line"); break; }
	const auto trcnridx = start % worktrcnrsz;

	const auto linenr = workhss.lineNr4Idx( lineidx );
	const auto trcnr = workhss.trcNr4Idx( trcnridx );
	const auto inlineidx = inhss.idx4LineNr( linenr );
	const auto outlineidx = outhss.idx4LineNr( linenr );
	const auto intrcidx = inhss.idx4TrcNr( trcnr );
	const auto outtrcidx = outhss.idx4TrcNr( trcnr );

	if ( domemcopy_ )
	{
	    const od_int64 inoffset = inlineidx*inlinebytes_ +
				      intrcidx*intracebytes_;
	    const od_int64 outoffset = outlineidx*outlinebytes_ +
				       outtrcidx*outtracebytes_;

	    for ( int idc=0; idc<nrcomps; idc++ )
	    {
		const unsigned char* curinptr = inptr_[idc] + inoffset;
		unsigned char* curoutptr = outptr_[idc] + outoffset;
		if ( inzidxs && outzidxs )
		{
		    for ( int idz=0; idz<nrz; idz++ )
		    {
			curinptr += inzidxs[idz];
			curoutptr += outzidxs[idz];
			OD::sysMemCopy( curoutptr, curinptr, samplebytes_ );
		    }
		}
		else
		    OD::sysMemCopy( curoutptr, curinptr, bytestocopy_ );
	    }
	}
	else
	{
	    for ( int idz=0; idz<nrz; idz++ )
	    {
		const int inzidx = inzidxs[idz];
		const int outzidx = outzidxs[idz];

		for ( int idc=0; idc<nrcomps; idc++ )
		{
		    const float val =
			in_.data(incomp_[idc]).get(inlineidx,intrcidx,inzidx);
		    if ( mFastIsFloatDefined(val) )
			out_.data(idc).set(outlineidx,outtrcidx,outzidx,val);
		}
	    }
	}
    }

    delete [] inzidxs;
    delete [] outzidxs;
    return true;
}


protected:

    const RegularSeisDataPack&	in_;
    RegularSeisDataPack&	out_;
    Survey::GeomSubSel*		worksubsel_;

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
				!reqss_.isZSlice(), reqss_.is2D() );
    RegularSeisDataPack* output =
	new RegularSeisDataPack( category, &packset[0]->getDataDesc() );
    DPM(DataPackMgr::SeisID()).add( output );
    if ( packset[0]->getScaler() )
	output->setScaler( *packset[0]->getScaler() );

    if ( cache_ && cache_->subSel().zRange().step != reqss_.zSubSel().zStep() )
    {
	PtrMan<Survey::GeomSubSel> gss = reqss_.getGeomSubSel();
	auto zrg = gss->zRange();
	zrg.step = cache_->subSel().zRange().step;
	gss->setZRange( zrg );
	output->setSubSel( *gss );
    }
    else
    {
	// For running dimensions, steps of subsel (Inl, Crl and Z)
	// should be same as that of datapack (packset) that is preloaded.
	PtrMan<Survey::GeomSubSel> avss = packset[0]->subSel().duplicate();
	for ( int idx=1; idx<packset.size(); idx++ )
	    avss->merge( packset[idx]->subSel() );

	PtrMan<Survey::GeomSubSel> gss = reqss_.getGeomSubSel();
	gss->limitTo( *avss );
	output->setSubSel( *gss );
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
    reqss_ = fss;
}


void EngineMan::setSubSel( const GeomSubSel& gss )
{
    if ( attrset_ && gss.is2D() != attrset_->is2D() )
	{ pErrMsg("2D/3D mismatch"); return; }
    reqss_ = FullSubSel( gss );
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


Processor* EngineMan::createScreenOutput2D( uiRetVal& uirv,
					    Data2DHolder& output )
{
    Processor* proc = getProcessor( uirv );
    if ( !proc )
	return 0;

    TwoDOutput* attrout = new TwoDOutput( reqss_.trcNrRange(),
					  reqss_.zRange(), reqss_.geomID(0) );
    attrout->setOutput( output );
    proc->addOutput( attrout );

    return proc;
}


Processor* EngineMan::createDataPackOutput( uiRetVal& uirv,
					    const RegularSeisDataPack* prev )
{
    unRefAndZeroPtr( cache_ );
    if ( prev )
	{ cache_ = prev; cache_->ref(); }

#define mAddAttrOut( fss ) \
{ \
    DataPackOutput* attrout = new DataPackOutput( fss ); \
    attrout->setGeometry( fss ); \
    attrout->setUndefValue( udfval_ ); \
    proc->addOutput( attrout ); \
}

    Processor* proc = getProcessor( uirv );
    if ( !proc )
	return 0;

    const bool is2d = reqss_.is2D();
    FullSubSel todofss( reqss_ );
    if ( !cache_ )
	mAddAttrOut( todofss )
    else
    {
	const auto& cachess = cache_->subSel();
	const auto& cachehss = cachess.horSubSel();
	const auto cacheinlrg = cachehss.lineNrRange();

	if ( !cachess.is2D() )
	{
	    const auto reqchss = reqss_.cubeHorSubSel();
	    const auto reqinlrg = reqchss.inlRange();
	    if ( cacheinlrg.start > reqinlrg.start )
	    {
		auto toloadrg( reqinlrg );
		toloadrg.stop = cacheinlrg.start - reqinlrg.step;
		FullSubSel fss( reqss_ );
		fss.setInlRange( toloadrg );
		mAddAttrOut( fss )
	    }
	    if ( cacheinlrg.stop < reqinlrg.stop )
	    {
		auto toloadrg( reqinlrg );
		toloadrg.start = cacheinlrg.stop + reqinlrg.step;
		FullSubSel fss( reqss_ );
		fss.setInlRange( toloadrg );
		mAddAttrOut( fss )
	    }
	}

	const auto cachetrcnrrg = cachehss.trcNrRange();
	const auto& reqhss = reqss_.horSubSel();
	const auto reqtrcnrrg = reqhss.trcNrRange();
	if ( cachetrcnrrg.start > reqtrcnrrg.start )
	{
	    auto toloadrg( reqtrcnrrg );
	    toloadrg.stop = cachetrcnrrg.start - reqtrcnrrg.step;
	    FullSubSel fss( reqss_ );
	    if ( !is2d )
		fss.setInlRange( cachetrcnrrg );
	    fss.setCrlRange( toloadrg );
	    mAddAttrOut( fss )
	}
	if ( cachetrcnrrg.stop < reqtrcnrrg.stop )
	{
	    auto toloadrg( reqtrcnrrg );
	    toloadrg.start = cachetrcnrrg.stop + reqtrcnrrg.step;
	    FullSubSel fss( reqss_ );
	    if ( !is2d )
		fss.setInlRange( cachetrcnrrg );
	    fss.setCrlRange( toloadrg );
	    mAddAttrOut( fss )
	}

	const auto cachezrg = cachess.zRange();
	const auto reqzrg = reqss_.zRange();
	const auto zeps = mZRelEps * cachezrg.step;
	if ( reqzrg.start < cachezrg.atIndex(-1) + zeps )
	{
	    auto toloadrg( reqzrg );
	    toloadrg.stop = cachezrg.start - reqzrg.step;
	    FullSubSel fss( cachess );
	    fss.setZRange( toloadrg );
	    mAddAttrOut( fss )
	}
	if ( reqzrg.stop > cachezrg.atIndex(cachezrg.nrSteps()+1) - zeps )
	{
	    auto toloadrg( reqzrg );
	    toloadrg.start = cachezrg.stop + reqzrg.step;
	    FullSubSel fss( cachess );
	    fss.setZRange( toloadrg );
	    mAddAttrOut( fss )
	}
    }

    if ( !is2d && reqss_.isFlat() && !reqss_.isZSlice() )
    {
	const auto css = reqss_.cubeSubSel();
	TypeSet<BinID> positions;
	if ( css.defaultDir() == OD::InlineSlice )
	{
	    const auto inl = reqss_.inlRange().start;
	    const auto crlrg = reqss_.crlRange();
	    const auto sz = crlrg.nrSteps() + 1;
	    for ( int idx=0; idx<sz; idx++ )
		positions += BinID( inl, crlrg.atIndex(idx) );
	}
	else if ( css.defaultDir() == OD::CrosslineSlice )
	{
	    const auto crl = reqss_.crlRange().start;
	    const auto inlrg = reqss_.inlRange();
	    const auto sz = inlrg.nrSteps() + 1;
	    for ( int idx=0; idx<sz; idx++ )
		positions += BinID( inlrg.atIndex(idx), crl );
	}

	proc->setRdmPaths( positions, positions );
    }

    return proc;
}


class AEMFeatureExtracter : public Executor
{ mODTextTranslationClass(AEMFeatureExtracter);
public:
AEMFeatureExtracter( EngineMan& aem, const BufferStringSet& inputs,
		     const ObjectSet<BinnedValueSet>& bivsets )
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

    ObjectSet<BinnedValueSet>& bvs =
	const_cast<ObjectSet<BinnedValueSet>&>(bivsets);

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
				    const ObjectSet<BinnedValueSet>& bivsets )
{
    return new AEMFeatureExtracter( *this, inputs, bivsets );
}


void EngineMan::computeIntersect2D( ObjectSet<BinnedValueSet>& bivsets ) const
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

    ObjectSet<BinnedValueSet> newbivsets;
    for ( int idx=0; idx<bivsets.size(); idx++ )
    {
	auto* newset = new BinnedValueSet(bivsets[idx]->nrVals(), true);
	ObjectSet<PosInfo::LineSet2DData::IR> resultset;
	linesetgeom.intersect( *bivsets[idx], resultset );

	for ( int idy=0; idy<resultset.size(); idy++)
	    newset->append(*resultset[idy]->posns_);

	newbivsets += newset;
    }
    bivsets = newbivsets;
}


Processor* EngineMan::createLocationOutput( uiRetVal& uirv,
					ObjectSet<BinnedValueSet>& bidzvset )
{
    if ( bidzvset.size() == 0 ) return 0;

    Processor* proc = getProcessor( uirv );
    if ( !proc )
	return 0;

    computeIntersect2D(bidzvset);
    ObjectSet<LocationOutput> outputs;
    for ( int idx=0; idx<bidzvset.size(); idx++ )
    {
	BinnedValueSet& bidzvs = *bidzvset[idx];
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

    ObjectSet<BinnedValueSet> bidsets;
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
					  const BinnedValueSet& bidvalset,
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
