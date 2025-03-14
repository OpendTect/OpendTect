/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seistrctr.h"

#include "bufstringset.h"
#include "envvars.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "od_stream.h"
#include "plugins.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seisinfo.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seistrc.h"
#include "separstr.h"
#include "sorting.h"
#include "survinfo.h"
#include "tracedata.h"
#include "trckeyzsampling.h"

#include <math.h>


const char* SeisTrcTranslator::sKeySeisTrPars() { return "SeisTranslator"; }
const char* SeisTrcTranslator::sKeyIs2D()	{ return "Is2D"; }
const char* SeisTrcTranslator::sKeyIsPS()	{ return "IsPS"; }
const char* SeisTrcTranslator::sKeyRegWrite()
					{ return "Enforce Regular Write"; }
const char* SeisTrcTranslator::sKeySIWrite()
					{ return "Enforce SurveyInfo Write"; }


SeisTrcTranslator::ComponentData::ComponentData( const char* nm )
    : BasicComponentInfo(nm)
{}


SeisTrcTranslator::ComponentData::ComponentData( const ComponentData& cd )
    : BasicComponentInfo(cd)
{}


SeisTrcTranslator::ComponentData::ComponentData( const SeisTrc& trc, int icomp,
						 const char* nm )
	: BasicComponentInfo(nm)
{
    datachar_ = trc.data().getInterpreter(icomp)->dataChar();
}


SeisTrcTranslator::ComponentData::~ComponentData()
{}


SeisTrcTranslator::TargetComponentData::TargetComponentData(
					    const ComponentData& c, int idx )
    : ComponentData(c)
    , destidx(idx)
    , org(c)
{}


SeisTrcTranslator::TargetComponentData::~TargetComponentData()
{}


const char*
SeisTrcTranslatorGroup::getSurveyDefaultKey( const IOObj* ioobj ) const
{
    static BufferString defkey =
			IOPar::compKey( sKey::Default(), sKeyDefault3D() );
    if ( !ioobj )
	return defkey.buf();

    BufferString type = ioobj->pars().find( sKey::Type() );
    // tweak to club thinned and unthinned FL together
    const char* fltype = sKey::FaultLikelihood();
    if ( type.contains(fltype) )
	type = fltype;

    return type.isEmpty() || type == sKey::Attribute() ? defkey.buf()
					: IOPar::compKey( defkey, type );
}


SeisTrcTranslator::SeisTrcTranslator( const char* nm, const char* unm )
    : Translator(nm,unm)
    , pinfo_(*new SeisPacketInfo)
    , enforce_regular_write( !GetEnvVarYN("OD_NO_SEISWRITE_REGULARISATION") )
    , enforce_survinfo_write( GetEnvVarYN("OD_ENFORCE_SURVINFO_SEISWRITE") )
    , trcblock_(*new SeisTrcBuf(false))
    , warnings_(*new BufferStringSet)
    , lastinlwritten_(SI().sampling(false).hsamp_.start_.inl())
{
}


SeisTrcTranslator::~SeisTrcTranslator()
{
    cleanUp();
    delete compnms_;
    delete &trcblock_;
    delete &pinfo_;
    delete &warnings_;
}


bool SeisTrcTranslator::is2D( const IOObj& ioobj, bool internal_only )
{
    const OD::String& grpnm = ioobj.group();
    const bool trok = !grpnm.isEmpty() && grpnm[0] == '2';
    return trok || internal_only ? trok
	: ioobj.pars().isTrue( sKeyIs2D() ) || Seis::is2DGeom( ioobj.pars() );
}


bool SeisTrcTranslator::isPS( const IOObj& ioobj, bool internal_only )
{
    const OD::String& grpnm = ioobj.group();
    const bool trok = (!grpnm.isEmpty() && grpnm[0] == 'P') ||
				(grpnm.size()>3 && grpnm[3] == 'P');
    if ( trok || internal_only )
	return trok;

    return ioobj.pars().isTrue(sKeyIsPS()) || Seis::isPSGeom(ioobj.pars());
}


bool SeisTrcTranslator::isLineSet( const IOObj& ioobj )
{
    return *ioobj.translator() == '2';
}


void SeisTrcTranslator::cleanUp()
{
    close();
    deleteAndNullPtr( conn_ );

    headerdonenew_ = false;
    datareaddone_ = false;
    deleteAndNullPtr( storbuf_ );
    deepErase( cds_ );
    deepErase( tarcds_ );
    deleteAndNullArrPtr( inpfor_ );
    deleteAndNullArrPtr( inpcds_ );
    deleteAndNullArrPtr( outcds_ );
    deleteAndNullPtr( trcscalebase_ );
    curtrcscalebase_ = nullptr;
    nrout_ = 0;
    errmsg_.setEmpty();
}


bool SeisTrcTranslator::close()
{
    bool ret = true;
    if ( !forRead() )
    {
	ret = writeBlock();
	if ( ret && conn_ )
	{
	    PtrMan<IOObj> obj = IOM().get( conn_->linkedTo() );
	    if ( obj )
		IOM().implUpdated.trigger( obj->key() );
	}
    }

    return ret;
}


bool SeisTrcTranslator::initRead( Conn* c, Seis::ReadMode rm )
{
    cleanUp();
    read_mode = rm;
    if ( !initConn(c) || !initRead_() )
    {
	deleteAndNullPtr( conn_ );
	return false;
    }

    pinfo_.zrg_.start_ = insd_.start_;
    pinfo_.zrg_.step_ = insd_.step_;
    pinfo_.zrg_.stop_ = insd_.start_ + insd_.step_ * (innrsamples_-1);
    return true;
}


bool SeisTrcTranslator::initWrite( Conn* c, const SeisTrc& trc )
{
    cleanUp();

    innrsamples_ = outnrsamples_ = trc.size();
    if ( innrsamples_ < 1 )
    {
	errmsg_ = tr("Empty first trace");
	return false;
    }

    insd_ = outsd_ = trc.info().sampling_;

    if ( seldata_ )
    {
	pinfo_.inlrg_.setInterval( seldata_->inlRange() );
	pinfo_.crlrg_.setInterval( seldata_->crlRange() );
	pinfo_.zrg_.setInterval( seldata_->zRange() );
    }

    if ( !initConn(c) || !initWrite_(trc) )
    {
	deleteAndNullPtr( conn_ );
	return false;
    }

    return true;
}


bool SeisTrcTranslator::commitSelections()
{
    errmsg_ = tr("No selected components found");
    const int sz = tarcds_.size();
    if ( sz < 1 )
	return false;

    outsd_ = insd_;
    outnrsamples_ = innrsamples_;
    if ( seldata_ && !mIsUdf(seldata_->zRange().start_) )
    {
	Interval<float> selzrg( seldata_->zRange() );
	const Interval<float> sizrg( SI().sampling(false).zsamp_ );
	if ( !mIsEqual(selzrg.start_,sizrg.start_,1e-8)
	     || !mIsEqual(selzrg.stop_,sizrg.stop_,1e-8) )
	{
	    outsd_.start_ = selzrg.start_;
	    const float fnrsteps = (selzrg.stop_-selzrg.start_) / outsd_.step_;
	    outnrsamples_ = mNINT32(fnrsteps) + 1;
	}
    }

    mAllocLargeVarLenArr( int, selnrs, sz );
    mAllocLargeVarLenArr( int, inpnrs, sz );
    int nrsel = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	int destidx = tarcds_[idx]->destidx;
	if ( destidx >= 0 )
	{
	    selnrs[nrsel] = destidx;
	    inpnrs[nrsel] = idx;
	    nrsel++;
	}
    }

    nrout_ = nrsel;
    if ( nrout_ < 1 )
	nrout_ = 1;

    delete [] inpfor_;
    inpfor_ = new int [nrout_];
    if ( nrsel < 1 )
	inpfor_[0] = 0;
    else if ( nrsel == 1 )
	inpfor_[0] = inpnrs[0];
    else
    {
	int* selnrsptr = selnrs.ptr();
	int* inpnrsptr = inpnrs.ptr();
	sort_coupled( selnrsptr, inpnrsptr, nrsel );
	for ( int idx=0; idx<nrout_; idx++ )
	    inpfor_[idx] = inpnrs[idx];
    }

    delete [] inpcds_;
    inpcds_ = new ComponentData* [nrout_];
    delete [] outcds_;
    outcds_ = new TargetComponentData* [nrout_];
    for ( int idx=0; idx<nrout_; idx++ )
    {
	inpcds_[idx] = cds_[ selComp(idx) ];
	outcds_[idx] = tarcds_[ selComp(idx) ];
    }

    errmsg_.setEmpty();
    enforceBounds();

    float fsampnr = (outsd_.start_ - insd_.start_) / insd_.step_;
    samprg_.start_ = mNINT32( fsampnr );
    samprg_.stop_ = samprg_.start_ + outnrsamples_ - 1;

    const bool forread = forRead();
    const int nrcomps = nrSelComps();
    const ComponentData** cds = forread ? (const ComponentData**)inpcds_
					: (const ComponentData**)outcds_;
    const int ns = forread ? innrsamples_ : outnrsamples_;
    delete storbuf_;
    storbuf_ = new TraceData;
    for ( int iselc=0; iselc<nrcomps; iselc++ )
	storbuf_->addComponent( ns+1, cds[iselc]->datachar_ , true );

    if ( !storbuf_->allOk() )
    {
	errmsg_ = tr("Out of memory");
	deleteAndNullPtr( storbuf_ );
	return false;
    }

    return commitSelections_();
}


void SeisTrcTranslator::enforceBounds()
{
    // Ranges
    outsd_.step_ = insd_.step_;
    float outstop = outsd_.start_ + (outnrsamples_ - 1) * outsd_.step_;
    if ( outsd_.start_ < insd_.start_ )
	outsd_.start_ = insd_.start_;
    const float instop = insd_.start_ + (innrsamples_ - 1) * insd_.step_;
    if ( outstop > instop )
	outstop = instop;

    // Snap to samples
    float sampdist = (outsd_.start_ - insd_.start_) / insd_.step_;
    int startsamp = (int)(sampdist + 0.0001);
    if ( startsamp < 0 )
	startsamp = 0;
    if ( startsamp > innrsamples_-1 )
	startsamp = innrsamples_-1;

    sampdist = (outstop - insd_.start_) / insd_.step_;
    int endsamp = (int)(sampdist + 0.9999);
    if ( endsamp < startsamp )
	endsamp = startsamp;
    if ( endsamp > innrsamples_-1 )
	endsamp = innrsamples_-1;

    outsd_.start_ = insd_.start_ + startsamp * insd_.step_;
    outnrsamples_ = endsamp - startsamp + 1;
}


bool SeisTrcTranslator::write( const SeisTrc& trc )
{
    if ( !inpfor_ && !commitSelections() )
	return false;

    if ( !inlCrlSorted() )
    {
	// No buffering: who knows what we'll get?
	dumpBlock();
	return writeTrc_( trc );
    }

    const bool haveprev = !mIsUdf( prevnr_ );
    const bool wrblk = haveprev && (is_2d ? prevnr_ > 99
				: prevnr_ != trc.info().inl());
    if ( !is_2d )
	prevnr_ = trc.info().inl();
    else if ( wrblk || !haveprev )
	prevnr_ = 1;
    else
	prevnr_++;

    if ( wrblk && !writeBlock() )
	return false;

    SeisTrc* newtrc; mTryAlloc( newtrc, SeisTrc(trc) );
    if ( !newtrc )
    {
	errmsg_ = tr("Out of memory");
	trcblock_.deepErase();
	return false;
    }

    trcblock_.add( newtrc );
    return true;
}


bool SeisTrcTranslator::writeBlock()
{
    int sz = trcblock_.size();
    SeisTrc* firsttrc = sz ? trcblock_.get(0) : nullptr;
    if ( firsttrc )
	lastinlwritten_ = firsttrc->info().inl();

    if ( sz && enforce_regular_write )
    {
	SeisTrcInfo::Fld keyfld = is_2d ? SeisTrcInfo::TrcNr
					: SeisTrcInfo::BinIDCrl;
	bool sort_asc = true;
	if ( is_2d )
	    sort_asc = trcblock_.get(0)->info().trcNr()
		     < trcblock_.get(trcblock_.size()-1)->info().trcNr();
	    // for 2D we're buffering only 100 traces, not an entire line
	trcblock_.sort( sort_asc, keyfld );
	firsttrc = trcblock_.get( 0 );
	const int firstnr = firsttrc->info().trcNr();
	int nrperpos = 1;
	if ( !is_2d )
	{
	    for ( int idx=1; idx<sz; idx++ )
	    {
		if ( trcblock_.get(idx)->info().crl() != firstnr )
		    break;
		nrperpos++;
	    }
	}

	if ( !isPS() )
	    trcblock_.enforceNrTrcs( nrperpos, keyfld );
	sz = trcblock_.size();
    }

    if ( !enforce_survinfo_write )
	return dumpBlock();

    StepInterval<int> inlrg, crlrg;
    SI().sampling(true).hsamp_.get( inlrg, crlrg );
    const int firstafter = crlrg.stop_ + crlrg.step_;
    int stp = crlrg.step_;
    int bufidx = 0;
    SeisTrc* trc = bufidx < sz ? trcblock_.get(bufidx) : nullptr;
    BinID binid( lastinlwritten_, crlrg.start_ );
    SeisTrc* filltrc = nullptr;
    int nrwritten = 0;
    for ( ; binid.crl() != firstafter; binid.crl() += stp )
    {
	while ( trc && trc->info().crl() < binid.crl() )
	{
	    bufidx++;
	    trc = bufidx < sz ? trcblock_.get(bufidx) : 0;
	}
	if ( trc )
	{
	    if ( !writeTrc_(*trc) )
		return false;
	}
	else
	{
	    if ( !filltrc )
		filltrc = getFilled( binid );
	    else
	    {
		if ( is_2d )
		    filltrc->info().setGeomID( Pos::GeomID(binid.row()) )
				   .setTrcNr( binid.trcNr() );
		else
		    filltrc->info().setPos( binid );

		filltrc->info().calcCoord();
	    }
	    if ( !writeTrc_(*filltrc) )
		return false;
	}
	nrwritten++;
    }

    delete filltrc;
    trcblock_.deepErase();
    blockDumped( nrwritten );
    return true;
}


bool SeisTrcTranslator::dumpBlock()
{
    bool rv = true;
    const int sz = trcblock_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !writeTrc_(*trcblock_.get(idx)) )
	{
	    rv = false;
	    break;
	}
    }

    trcblock_.deepErase();
    blockDumped( sz );
    return rv;
}


void SeisTrcTranslator::prepareComponents( SeisTrc& trc, int actualsz ) const
{
    for ( int idx=0; idx<nrout_; idx++ )
    {
	TraceData& td = trc.data();
	if ( !tarcds_.validIdx(idx) )
	    break;

	if ( td.nrComponents() <= idx )
	    td.addComponent( actualsz, tarcds_[ inpfor_[idx] ]->datachar_ );
	else
	{
	    td.setComponent( tarcds_[ inpfor_[idx] ]->datachar_, idx );
	    td.reSize( actualsz, idx );
	}
    }
}


bool SeisTrcTranslator::forRead() const
{
    return conn_ ? conn_->forRead() : true;
}



void SeisTrcTranslator::addComp( const DataCharacteristics& dc,
				 const char* nm, int dtype )
{
    BufferString str( "Component " );
    if ( !nm || !*nm )
    {
	if ( compnms_ && cds_.size() < compnms_->size() )
	    nm = compnms_->get(cds_.size()).buf();
	else
	{
	    str += cds_.size() + 1;
	    nm = str.buf();
	}
    }

    auto* newcd = new ComponentData( nm );
    newcd->datachar_ = dc;
    newcd->datatype_ = dtype;
    cds_ += newcd;
    bool isl = newcd->datachar_.littleendian_;
    newcd->datachar_.littleendian_ = __islittle__;
    tarcds_ += new TargetComponentData( *newcd, cds_.size()-1 );
    newcd->datachar_.littleendian_ = isl;
}


bool SeisTrcTranslator::initConn( Conn* c )
{
    close();
    errmsg_.setEmpty();
    if ( !c )
    {
	errmsg_ = tr("Translator: No connection established");
	objstatus_ = IOObj::Status::FileNotPresent;
	return false;
    }

    mDynamicCastGet(StreamConn*,strmconn,c)
    if ( strmconn )
    {
	BufferString fnm = strmconn->odStream().fileName();
	if ( c->isBad() && !File::isDirectory(fnm) )
	{
	    if ( !fnm.isEmpty() && !File::exists(fnm) )
	    {
		errmsg_ = tr( "File doesn't exist: %1" ).arg( fnm );
		objstatus_ = IOObj::Status::FileNotPresent;
		return false;
	    }

	    errmsg_ = tr( "Cannot open file: %1" ).arg( fnm );
	    objstatus_ = IOObj::Status::ReadPermissionInvalid;
	    return false;
	}
    }

    delete conn_;
    conn_ = c;
    return true;
}


SeisTrc* SeisTrcTranslator::getEmpty()
{
    DataCharacteristics dc;
    if ( outcds_ )
	dc = outcds_[0]->datachar_;
    else if ( !tarcds_.isEmpty() && inpfor_ )
	dc = tarcds_[selComp()]->datachar_;

    return new SeisTrc( 0, dc );
}


void SeisTrcTranslator::setComponentNames( const BufferStringSet& bss )
{
    delete compnms_;
    compnms_ = new BufferStringSet( bss );
}


void SeisTrcTranslator::getComponentNames( BufferStringSet& bss ) const
{
    bss.erase();
    if ( cds_.size() == 1 )	//TODO display comp name only if more than 1
    {
	bss.add("");
	return;
    }

    for ( int idx=0; idx<cds_.size(); idx++ )
	bss.add( cds_[idx]->name() );
}


bool SeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !headerdonenew_ && !readInfo(trc.info()) )
	return false;

    if ( (!datareaddone_ && !readData()) || !copyDataToTrace(trc) )
	return false;

    headerdonenew_ = false;
    datareaddone_ = false;

    return true;
}


bool SeisTrcTranslator::copyDataToTrace( SeisTrc& trc )
{
    if ( !datareaddone_ )
	return false;

    prepareComponents( trc, outnrsamples_ );
    if ( curtrcscalebase_ )
	trc.convertToFPs();

    const int nrcomps = nrSelComps();
    if ( nrcomps == 0 )
	return false;

    TraceDataInterpreter* trcdatainterp = trc.data().getInterpreter( 0 );
    TraceDataInterpreter* storinterp =
	storbuf_ ? storbuf_->getInterpreter( 0 ) : nullptr;
    if ( !trcdatainterp || !storinterp )
	return false;

    const bool matchingdc = *trcdatainterp == *storinterp;
    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	if ( matchingdc && outnrsamples_ > 1 && !curtrcscalebase_ )
	{
	    const int nrbytes = outnrsamples_ * storbuf_->bytesPerSample(iselc);
	    OD::memCopy( trc.data().getComponent(iselc)->data(),
			 storbuf_->getComponent(iselc)->data(), nrbytes );
	}
	else
	{
	    for ( int isamp=0; isamp<outnrsamples_; isamp++ )
	    {
		//Parallel !!
		float inpval = storbuf_->getValue( isamp, iselc );
		if ( curtrcscalebase_ )
		    inpval = (float)curtrcscalebase_->scale(inpval);

		trc.set( isamp, inpval, iselc );
	    }
	}
    }

    return true;
}


SeisTrc* SeisTrcTranslator::getFilled( const BinID& binid )
{
    if ( !outcds_ )
	return nullptr;

    auto* newtrc = new SeisTrc;
    if ( is_2d )
	newtrc->info().setGeomID( Pos::GeomID(binid.row()) )
		      .setTrcNr( binid.trcNr() );
    else
	newtrc->info().setPos( binid );

    newtrc->info().calcCoord();

    newtrc->data().delComponent(0);
    for ( int idx=0; idx<nrout_; idx++ )
    {
	newtrc->data().addComponent( outnrsamples_,
				     outcds_[idx]->datachar_, true );
	newtrc->info().sampling_ = outsd_;
    }

    return newtrc;
}


bool SeisTrcTranslator::getRanges( const MultiID& ky, TrcKeyZSampling& cs )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    return ioobj ? getRanges( *ioobj, cs ) : false;
}


bool SeisTrcTranslator::getRanges( const IOObj& ioobj, TrcKeyZSampling& cs )
{
    PtrMan<Translator> transl = ioobj.createTranslator();
    mDynamicCastGet(SeisTrcTranslator*,tr,transl.ptr());
    if ( !tr )
	return false;

    PtrMan<Seis::SelData> sd;
    if ( cs.is2D() )
    {
	sd = Seis::SelData::get( Seis::Range );
	sd->setGeomID( cs.hsamp_.getGeomID() );
	tr->setSelData( sd.ptr() );
    }

    mDynamicCastGet(const IOStream*,iostrmptr,&ioobj)
    if ( !iostrmptr || !iostrmptr->isMultiConn() )
    {
	Conn* cnn = ioobj.getConn( Conn::Read );
	if ( !cnn || !tr->initRead(cnn,Seis::PreScan) )
	    return false;

	const SeisPacketInfo& pinf = tr->packetInfo();
	cs.hsamp_.set( pinf.inlrg_, pinf.crlrg_ );
	cs.zsamp_ = pinf.zrg_;
    }
    else
    {
	IOStream& iostrm = *const_cast<IOStream*>( iostrmptr );
	iostrm.resetConnIdx();
	cs.setEmpty();
	do
	{
	    PtrMan<Translator> translator = ioobj.createTranslator();
	    mDynamicCastGet(SeisTrcTranslator*,seistr,translator.ptr());
	    Conn* conn = iostrm.getConn( Conn::Read );
	    if ( !seistr || !conn || !seistr->initRead(conn,Seis::PreScan) )
		return !cs.isEmpty();

	    const SeisPacketInfo& pinf = seistr->packetInfo();
	    TrcKeyZSampling newcs( false );
	    newcs.hsamp_.set( pinf.inlrg_, pinf.crlrg_ );
	    newcs.zsamp_ = pinf.zrg_;
	    cs.include( newcs );
	} while ( iostrm.toNextConnIdx() );
    }

    return true;
}


void SeisTrcTranslator::usePar( const IOPar& iop )
{
    iop.getYN( sKeyRegWrite(), enforce_regular_write );
    iop.getYN( sKeySIWrite(), enforce_survinfo_write );

    PtrMan<IOPar> seistrpar = iop.subselect( sKeySeisTrPars() );
    if ( !seistrpar )
	seistrpar = new IOPar( iop );

    if ( seistrpar && !seistrpar->getYN(sKeyIsPS(),is_prestack) )
	is_prestack = Seis::isPSGeom( *seistrpar );

    if ( !seistrpar->getYN(sKeyIs2D(),is_2d) )
	is_2d = Seis::is2DGeom( *seistrpar );

    Pos::GeomID gid;
    if ( seistrpar->get(sKey::GeomID(),gid) && Survey::isValidGeomID(gid) )
	setCurGeomID( gid );

    RefMan<Coords::CoordSystem> crs =
			Coords::CoordSystem::createSystem( *seistrpar.ptr() );
    if ( crs )
	setCoordSys( *crs.ptr() );
}


void SeisTrcTranslator::setCurGeomID( Pos::GeomID gid )
{
    geomid_ = gid;
    is_2d = Survey::is2DGeom( geomid_ );
}


void SeisTrcTranslator::getAllFileNames( BufferStringSet& filenms,
					 bool forui ) const
{
    if ( forui )
	return;

    if ( havePars() )
    {
	const BufferString parfilename = getAuxFileName( sParFileExtension() );
	if ( !parfilename.isEmpty() )
	    filenms.addIfNew( parfilename );
    }

    if ( haveStats() )
    {
	const BufferString statsfilename
				    = getAuxFileName( sStatsFileExtension() );
	if ( !statsfilename.isEmpty() )
	    filenms.addIfNew( statsfilename );
    }

    if ( haveProc() )
    {
	const BufferString procfilename
				    = getAuxFileName( sProcFileExtension() );
	if ( !procfilename.isEmpty() )
	    filenms.addIfNew( procfilename );
    }
}


bool SeisTrcTranslator::havePars() const
{
    return haveAux( sParFileExtension() );
}


bool SeisTrcTranslator::haveStats() const
{
    return haveAux( sStatsFileExtension() );
}


bool SeisTrcTranslator::haveProc() const
{
    return haveAux( sProcFileExtension() );
}


bool SeisTrcTranslator::haveWarnings() const
{
    return !warnings_.isEmpty();
}


void SeisTrcTranslator::addWarn( int nr, const char* msg )
{
    if ( !msg || !*msg || warnnrs_.isPresent(nr) )
	return;

    warnnrs_ += nr;
    warnings_.add( msg );
}


void SeisTrcTranslator::setCoordSys( const Coords::CoordSystem& crs )
{
    coordsys_ = &crs;
}


void SeisTrcTranslator::setType( Seis::GeomType geom, IOPar& iop )
{
    iop.setYN( sKeyIs2D(), Seis::is2D(geom) );
    iop.setYN( sKeyIsPS(), Seis::isPS(geom) );
}


void SeisTrcTranslator::setGeomID( Pos::GeomID gid, IOPar& iop )
{
    iop.set( sKey::GeomID(), gid );
}


void SeisTrcTranslator::setCoordSys( const Coords::CoordSystem& crs,
				     IOPar& iop )
{
    crs.fillPar( iop );
}


void Seis::addPluginTranslators()
{
    initPluginClasses( "SeisTrcTranslators", "InitSeisTrcTrl" );
}
