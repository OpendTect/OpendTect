/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
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
#include "iopar.h"
#include "ioobj.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seissingtrcproc.h"
#include "seiscbvs.h"
#include "seistrc.h"
#include "seisseldata.h"
#include "seisresampler.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


uiSeisTransfer::uiSeisTransfer( uiParent* p, const uiSeisTransfer::Setup& s )
	: uiGroup(p,"Seis transfer pars")
	, setup_(s)
	, trcgrowfld_(0)
	, issteer_(false)
{
    selfld = uiSeisSubSel::get( this, setup_ );

    uiStringSet choices;
    choices += uiStrings::sDiscard();
    choices += uiStrings::sPass();
    choices += uiStrings::sAdd();
    choices += uiString::empty();
    if ( !setup_.is2d_ && !setup_.isps_ && setup_.withnullfill_ )
	remnullfld = new uiGenInput( this, tr("Null traces"),
				     StringListInpSpec(choices) );
    else
	remnullfld = new uiGenInput( this, tr("Null traces"),
				     BoolInpSpec(true,choices[0],choices[1]) );
    remnullfld->attach( alignedBelow, selfld );

    scalefld_ = new uiScaler( this, uiString::empty(), true );
    scalefld_->attach( alignedBelow, remnullfld );

    if ( !setup_.fornewentry_ )
    {
	trcgrowfld_ = new uiGenInput(this, tr("Adjust Z range to survey range"),
					BoolInpSpec(false));
	trcgrowfld_->attach( alignedBelow, scalefld_ );
    }

    setHAlignObj( remnullfld );
    postFinalise().notify( mCB(this,uiSeisTransfer,updSteer) );
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
    setIsSteering( res && *res == 'S' );
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


void uiSeisTransfer::setIsSteering( bool yn )
{
    issteer_ = yn;
    updSteer( 0 );
}


void uiSeisTransfer::updSteer( CallBacker* )
{
    if ( issteer_ )
	scalefld_->setUnscaled();
}


Seis::SelData* uiSeisTransfer::getSelData() const
{
    if ( selfld->isAll() )
	return nullptr;
    IOPar iop;
    if ( !selfld->fillPar( iop ) ) return nullptr;
    return Seis::SelData::get( iop );
}


SeisResampler* uiSeisTransfer::getResampler() const
{
    if ( selfld->isAll() )
	return nullptr;

    TrcKeyZSampling cs;
    selfld->getSampling( cs.hsamp_ );
    selfld->getZRange( cs.zsamp_ );
    return new SeisResampler( cs );
}


Executor* uiSeisTransfer::getTrcProc( const IOObj& inobj,
				      const IOObj& outobj,
				      const char* extxt,
				      const uiString& worktxt,
				      const char* linenm2d ) const
{
    PtrMan<Seis::SelData> seldata = getSelData();
    IOPar iop;
    iop.set( "ID", inobj.key() );
    if ( seldata )
	seldata->fillPar( iop );
    else if ( setup_.is2d_ )
    {
	Pos::GeomID geomid = SurvGeom::getGeomID( linenm2d );
	iop.set( sKey::GeomID(), geomid );
    }

    SeisSingleTraceProc* stp = new SeisSingleTraceProc( inobj, outobj,
					extxt, &iop, worktxt );
    stp->setScaler( scalefld_->getScaler() );
    stp->skipNullTraces( removeNull() );
    stp->fillNullTraces( fillNull() );
    stp->setResampler( getResampler() );
    stp->setExtTrcToSI( extendTrcsToSI() );

    return stp;
}


void uiSeisTransfer::fillPar( IOPar& iop ) const
{
    selfld->fillPar( iop );
    scalefld_->fillPar( iop );

    iop.setYN( SeisTrc::sKeyExtTrcToSI(), extendTrcsToSI() );
    iop.set( sKeyNullTrcPol(), nullTrcPolicy() );
}
