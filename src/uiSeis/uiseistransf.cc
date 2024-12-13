/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseistransf.h"

#include "uimsg.h"
#include "uigeninput.h"
#include "uiscaler.h"
#include "uiseissubsel.h"

#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "seisresampler.h"
#include "seisselection.h"
#include "seissingtrcproc.h"
#include "seisstor.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "trckeyzsampling.h"


// uiSeisTransfer::Setup

uiSeisTransfer::Setup::Setup( Seis::GeomType gt )
    : Seis::SelSetup(gt)
    , withnullfill_(false)
    , withmultiz_(false)
{
}


uiSeisTransfer::Setup::Setup( bool _is2d, bool _isps )
    : Seis::SelSetup(_is2d,_isps)
    , withnullfill_(false)
    , withmultiz_(false)
{
}


uiSeisTransfer::Setup::Setup( const Seis::SelSetup& sss )
    : Seis::SelSetup(sss)
    , withnullfill_(false)
    , withmultiz_(false)
{
}


uiSeisTransfer::Setup::~Setup()
{
}



// uiSeisTransfer

uiSeisTransfer::uiSeisTransfer( uiParent* p, const uiSeisTransfer::Setup& ss )
    : uiGroup(p,"Seis transfer pars")
    , setup_(ss)
    , selChange(this)
    , nullValChange(this)
{
    if ( setup_.withmultiz_ )
    {
	multizselfld_ = new uiMultiZSeisSubSel( this, setup_ );
	mAttachCB( multizselfld_->selChange, uiSeisTransfer::selChangeCB );
    }
    else
    {
	selfld_ = uiSeisSubSel::get( this, setup_ );
	mAttachCB( selfld_->selChange, uiSeisTransfer::selChangeCB );
    }

    uiStringSet choices;
    choices += uiStrings::sDiscard();
    choices += uiStrings::sPass();
    choices += uiStrings::sAdd();
    if ( !setup_.is2d_ && !setup_.isps_ && setup_.withnullfill_ )
	remnullfld_ = new uiGenInput( this, tr("Null traces"),
				     StringListInpSpec(choices) );
    else
	remnullfld_ = new uiGenInput( this, tr("Null traces"),
				     BoolInpSpec(true,choices[0],choices[1]) );
    mAttachCB( remnullfld_->valueChanged,
				    uiSeisTransfer::removeNullValChangeCB );
    if ( multizselfld_ )
	remnullfld_->attach( alignedBelow, multizselfld_->attachObj() );
    else
	remnullfld_->attach( alignedBelow, selfld_ );

    scalefld_ = new uiScaler( this, uiString::empty(), true );
    scalefld_->attach( alignedBelow, remnullfld_ );

    if ( !setup_.fornewentry_ )
    {
	trcgrowfld_ = new uiGenInput(this, tr("Adjust Z range to survey range"),
					BoolInpSpec(false));
	trcgrowfld_->attach( alignedBelow, scalefld_ );
    }

    setHAlignObj( remnullfld_ );
    mAttachCB( postFinalize(), uiSeisTransfer::initGrpCB );
}


uiSeisTransfer::~uiSeisTransfer()
{
    detachAllNotifiers();
}


void uiSeisTransfer::initGrpCB( CallBacker* )
{
    updSteerCB( nullptr );
}


void uiSeisTransfer::showSubselFld( bool yn )
{
    if ( selfld_ )
	selfld_->display( yn );

    if ( multizselfld_ )
	multizselfld_->display( yn );
}


const uiSeisSubSel* uiSeisTransfer::selFld() const
{
    return getNonConst(*this).selFld();
}


uiSeisSubSel* uiSeisTransfer::selFld()
{
    if ( selfld_ )
	return selfld_;

    if ( multizselfld_ )
	return multizselfld_->getSelGrp();

    return nullptr;
}


const uiSeis2DSubSel* uiSeisTransfer::selFld2D() const
{
    return getNonConst(*this).selFld2D();
}


uiSeis2DSubSel* uiSeisTransfer::selFld2D()
{
    mDynamicCastGet(uiSeis2DSubSel*,ret,selFld())
    return ret;
}


const uiSeis3DSubSel* uiSeisTransfer::selFld3D() const
{
    return getNonConst(*this).selFld3D();
}


uiSeis3DSubSel* uiSeisTransfer::selFld3D()
{
    mDynamicCastGet(uiSeis3DSubSel*,ret,selFld())
    return ret;
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    setInput( ioobj );
}


void uiSeisTransfer::setInput( const IOObj& ioobj )
{
    if ( selfld_ )
	selfld_->setInput( ioobj );

    if ( multizselfld_ )
	multizselfld_->setInput( ioobj );

    const BufferString res = ioobj.pars().find( sKey::Type() );
    if ( !res.isEmpty() )
    {
	const BufferString firstchar( res[0] );
	setSteering( firstchar.isEqual("S") );
    }
    else
	setSteering( false );
}


void uiSeisTransfer::setInput( const TrcKeyZSampling& tkzs )
{
    if ( selfld_ )
	selfld_->setInput( tkzs );

    if ( multizselfld_ )
	multizselfld_->setInput( tkzs );
}


void uiSeisTransfer::setSelectedLine( const char* lnm )
{
    if ( !is2D() || !isSingleLine() )
	return;

    selFld2D()->setSelectedLine( lnm );
}


void uiSeisTransfer::setSelFldSensitive( bool yn )
{
    selFld()->setSensitive( yn );
}


od_int64 uiSeisTransfer::expectedNrTraces() const
{
    return selFld()->expectedNrTraces();
}


SeisIOObjInfo::SpaceInfo uiSeisTransfer::spaceInfo( int bps ) const
{
    const uiSeisSubSel* selfld = selFld();
    SeisIOObjInfo::SpaceInfo si( selfld->expectedNrSamples(),
				 selfld->expectedNrTraces(), bps );
    if ( setup_.is2d_ )
	si.expectednrtrcs = -1;

    return si;
}


const ZDomain::Info* uiSeisTransfer::zDomain() const
{
    return selFld()->zDomain();
}


Scaler* uiSeisTransfer::getScaler() const
{
    return scalefld_->getScaler();
}


void uiSeisTransfer::setNull( bool val )
{
    remnullfld_->setValue( val );
}


bool uiSeisTransfer::removeNull() const
{
    return setup_.withnullfill_ ? remnullfld_->getIntValue() == 0
				: remnullfld_->getBoolValue();
}


bool uiSeisTransfer::fillNull() const
{
    return remnullfld_->getIntValue() == 2;
}


bool uiSeisTransfer::extendTrcsToSI() const
{
    return trcgrowfld_ && trcgrowfld_->getBoolValue();
}


bool uiSeisTransfer::isSingleLine() const
{
    return selFld()->is2D() && selFld2D()->isSingLine();
}


BufferString uiSeisTransfer::selectedLine() const
{
    BufferString lnm;
    if ( selFld()->is2D() )
	lnm = selFld2D()->selectedLine();

    return lnm;
}


void uiSeisTransfer::setSteering( bool yn )
{
    issteer_ = yn;
    updSteerCB( nullptr );
}


void uiSeisTransfer::selChangeCB( CallBacker* )
{
    selChange.trigger();
}


void uiSeisTransfer::removeNullValChangeCB( CallBacker* )
{
    nullValChange.trigger();
}


void uiSeisTransfer::updSteerCB( CallBacker* )
{
    if ( issteer_ )
	scalefld_->setUnscaled();
}


void uiSeisTransfer::setOutputHeader( const char* hdrtxt )
{
    outheader_.set( hdrtxt );
}


void uiSeisTransfer::setCoordSystem( const Coords::CoordSystem& crs, bool inp )
{
    if ( inp )
	inpcrs_ = &crs;
    else
	outpcrs_ = &crs;
}


bool uiSeisTransfer::is2D() const
{
    return selFld()->is2D();
}


Seis::SelData* uiSeisTransfer::getSelData() const
{
    IOPar iop;
    if ( (selfld_ && !selfld_->isDisplayed()) ||
	 (multizselfld_ && !multizselfld_->isDisplayed()) )
	return nullptr;

    const uiSeisSubSel* selfld = selFld();
    if ( !selfld->fillPar(iop) )
	return nullptr;

    auto* sd = Seis::SelData::get( iop );
    if ( setup_.is2d_ && sd->geomID() != curGeomID() )
	sd->setGeomID( curGeomID() );

    return sd;
}


SeisResampler* uiSeisTransfer::getResampler() const
{
    const uiSeisSubSel* selfld = selFld();
    if ( selfld->isAll() )
	return nullptr;

    TrcKeyZSampling cs;
    selfld->getSampling( cs.hsamp_ );
    selfld->getZRange( cs.zsamp_ );
    return new SeisResampler( cs, Seis::is2D(setup_.geomType()) );
}


Pos::GeomID uiSeisTransfer::curGeomID() const
{
    Pos::GeomID geomid;
    if ( selFld()->is2D() )
    {
	const uiSeis2DSubSel* sel2d = selFld2D();
	const BufferString linenm2d( sel2d->selectedLine() );
	geomid = Survey::GM().getGeomID( linenm2d );
    }
    else
	geomid = Survey::default3DGeomID();

    return geomid;
}


Executor* uiSeisTransfer::getTrcProc( const IOObj& inobj, const IOObj& outobj,
				      const char* extxt,const uiString& worktxt,
				      const char* linenm2d ) const
{
    if ( selFld()->is2D() && linenm2d && *linenm2d )
    {
	uiSeis2DSubSel* sel2d = getNonConst(*this).selFld2D();
	if ( sel2d && sel2d->isSingLine() )
	    sel2d->setSelectedLine( linenm2d );
    }

    return getTrcProc( inobj, outobj, extxt, worktxt );
}


Executor* uiSeisTransfer::getTrcProc( const IOObj& inobj, const IOObj& outobj,
				      const char* extxt,
				      const uiString& worktxt,
				      int compnr ) const
{
    const Seis::GeomType gt = setup_.geomType();
    const Pos::GeomID geomid = curGeomID();
    SeisStoreAccess::Setup inpsu( inobj, geomid, &gt );
    SeisStoreAccess::Setup outsu( outobj, geomid, &gt );
    inpsu.compnr( compnr );
    outsu.compnr( compnr ).hdrtxt( outheader_ );

    PtrMan<Seis::SelData> seldata = getSelData();
    if ( seldata && seldata->geomID() != geomid )
    {
	pErrMsg("Incompatible geomid");
	seldata->setGeomID( geomid );
    }

    if ( seldata && !seldata->isAll() )
    {
	inpsu.seldata( seldata.ptr() );
	outsu.seldata( seldata.ptr() );
    }

    if ( inpcrs_ )
	inpsu.coordsys( *inpcrs_.ptr() );
    if ( outpcrs_ )
	outsu.coordsys( *outpcrs_.ptr() );

    PtrMan<SeisSingleTraceProc> stp = new SeisSingleTraceProc( inpsu, outsu,
							       extxt, worktxt );
    if ( !stp->isOK() )
    {
	uiMSG().error( stp->errMsg() );
	return nullptr;
    }

    stp->setScaler( scalefld_->getScaler() );
    stp->skipNullTraces( removeNull() );
    stp->fillNullTraces( fillNull() );
    stp->setResampler( getResampler() );
    stp->setExtTrcToSI( extendTrcsToSI() );

    return stp.release();
}


void uiSeisTransfer::fillSelPar( IOPar& iop ) const
{
    selFld()->fillPar( iop );
}


void uiSeisTransfer::usePar( const IOPar& par )
{
    selfld_->usePar( par );
    scalefld_->usePar( par );
    ConstRefMan<Coords::CoordSystem> outcrs =
				    Coords::CoordSystem::createSystem( par );
    if ( outcrs && outcrs.ptr() != outpcrs_.ptr() )
	setCoordSystem( *outcrs.ptr(), true);

    bool extendtrctosi = false;
    par.getYN( SeisTrc::sKeyExtTrcToSI(), extendtrctosi );
    if ( trcgrowfld_ )
	trcgrowfld_->setValue( extendtrctosi );

    int nulltrcpol = mUdf(int);
    par.get( sKeyNullTrcPol(), nulltrcpol );
    remnullfld_->setValue( nulltrcpol );
}

void uiSeisTransfer::fillPar( IOPar& iop ) const
{
    SeisTrcTranslator::setType( setup_.geomType(), iop );
    SeisTrcTranslator::setGeomID( curGeomID(), iop );
    fillSelPar( iop );
    scalefld_->fillPar( iop );

    iop.setYN( SeisTrc::sKeyExtTrcToSI(), extendTrcsToSI() );
    iop.set( sKeyNullTrcPol(), nullTrcPolicy() );
    if ( !outheader_.isEmpty() )
	iop.set( SeisStoreAccess::sKeyHeader(), outheader_ );

    if ( outpcrs_ )
	outpcrs_->fillPar( iop );
}
