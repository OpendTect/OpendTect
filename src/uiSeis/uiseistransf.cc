/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          May 2002
________________________________________________________________________

-*/

#include "uiseistransf.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uigeninput.h"
#include "uiscaler.h"
#include "uimainwin.h"
#include "uimsg.h"

#include "conn.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "seisresampler.h"
#include "seisselection.h"
#include "seissingtrcproc.h"
#include "seisstor.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


uiSeisTransfer::uiSeisTransfer( uiParent* p, const uiSeisTransfer::Setup& s )
    : uiGroup(p,"Seis transfer pars")
    , setup_(s)
{
    selfld = uiSeisSubSel::get( this, setup_ );

    uiStringSet choices;
    choices += uiStrings::sDiscard();
    choices += uiStrings::sPass();
    choices += uiStrings::sAdd();
    if ( !setup_.is2d_ && !setup_.isps_ && setup_.withnullfill_ )
	remnullfld = new uiGenInput( this, tr("Null traces"),
				     StringListInpSpec(choices) );
    else
	remnullfld = new uiGenInput( this, tr("Null traces"),
				     BoolInpSpec(true,choices[0],choices[1]) );
    remnullfld->attach( alignedBelow, selfld );

    scalefld_ = new uiScaler( this, uiStrings::sEmptyString(), true );
    scalefld_->attach( alignedBelow, remnullfld );

    if ( !setup_.fornewentry_ )
    {
	trcgrowfld_ = new uiGenInput(this, tr("Adjust Z range to survey range"),
					BoolInpSpec(false));
	trcgrowfld_->attach( alignedBelow, scalefld_ );
    }

    setHAlignObj( remnullfld );
    mAttachCB( postFinalize(), uiSeisTransfer::updSteer );
}


uiSeisTransfer::~uiSeisTransfer()
{
    detachAllNotifiers();
}


void uiSeisTransfer::showSubselFld( bool showselfld )
{
    selfld->display( showselfld );
}


uiSeis2DSubSel* uiSeisTransfer::selFld2D()
{
    mDynamicCastGet(uiSeis2DSubSel*,ret,selfld)
    return ret;
}


uiSeis3DSubSel* uiSeisTransfer::selFld3D()
{
    mDynamicCastGet(uiSeis3DSubSel*,ret,selfld)
    return ret;
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    setInput( ioobj );
}


void uiSeisTransfer::setInput( const IOObj& ioobj )
{
    selfld->setInput( ioobj );

    const char* res = ioobj.pars().find( sKey::Type() );
    setSteering( res && *res == 'S' );
}


SeisIOObjInfo::SpaceInfo uiSeisTransfer::spaceInfo( int bps ) const
{
    SeisIOObjInfo::SpaceInfo si( selfld->expectedNrSamples(),
		selfld->expectedNrTraces(), bps );

    if ( setup_.is2d_ )
	si.expectednrtrcs = -1;

    return si;
}


Scaler* uiSeisTransfer::getScaler() const
{
    return scalefld_->getScaler();
}


bool uiSeisTransfer::removeNull() const
{
    return setup_.withnullfill_ ? remnullfld->getIntValue() == 0
				: remnullfld->getBoolValue();
}


bool uiSeisTransfer::fillNull() const
{
    return remnullfld->getIntValue() == 2;
}


bool uiSeisTransfer::extendTrcsToSI() const
{
    return trcgrowfld_ && trcgrowfld_->getBoolValue();
}


void uiSeisTransfer::setSteering( bool yn )
{
    issteer_ = yn;
    updSteer( nullptr );
}


void uiSeisTransfer::updSteer( CallBacker* )
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


Seis::SelData* uiSeisTransfer::getSelData() const
{
    IOPar iop;
    if ( !selfld || !selfld->isDisplayed() || !selfld->fillPar(iop) )
	return nullptr;

    auto* sd = Seis::SelData::get( iop );
    if ( setup_.is2d_ && sd->geomID() != curGeomID() )
	sd->setGeomID( curGeomID() );

    return sd;
}


SeisResampler* uiSeisTransfer::getResampler() const
{
    if ( selfld->isAll() )
	return nullptr;

    TrcKeyZSampling cs;
    selfld->getSampling( cs.hsamp_ );
    selfld->getZRange( cs.zsamp_ );
    return new SeisResampler( cs, Seis::is2D(setup_.geomType()) );
}


Pos::GeomID uiSeisTransfer::curGeomID() const
{
    Pos::GeomID geomid = mUdfGeomID;
    if ( setup_.is2d_ )
    {
	uiSeis2DSubSel* sel2d = const_cast<uiSeisTransfer*>( this )->selFld2D();
	const BufferString linenm2d( sel2d->selectedLine() );
	geomid = Survey::GM().getGeomID( linenm2d );
    }
    else if ( !setup_.is2d_ )
	geomid = Survey::default3DGeomID();

    return geomid;
}


Executor* uiSeisTransfer::getTrcProc( const IOObj& inobj, const IOObj& outobj,
				      const char* extxt,const uiString& worktxt,
				      const char* linenm2d ) const
{
    if ( linenm2d && *linenm2d )
    {
	uiSeis2DSubSel* sel2d = const_cast<uiSeisTransfer*>( this )->selFld2D();
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
	inpsu.seldata( seldata );
	outsu.seldata( seldata );
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


void uiSeisTransfer::fillPar( IOPar& iop ) const
{
    SeisTrcTranslator::setType( setup_.geomType(), iop );
    SeisTrcTranslator::setGeomID( curGeomID(), iop );
    selfld->fillPar( iop );
    scalefld_->fillPar( iop );

    iop.setYN( SeisTrc::sKeyExtTrcToSI(), extendTrcsToSI() );
    iop.set( sKeyNullTrcPol(), nullTrcPolicy() );
    if ( !outheader_.isEmpty() )
	iop.set( SeisStoreAccess::sKeyHeader(), outheader_ );

    if ( outpcrs_ )
	outpcrs_->fillPar( iop );
}
