/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiposprovider.h"

#include "uidialog.h"
#include "uigeninput.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uitoolbutton.h"

#include "ascstream.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "rangeposprovider.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


uiPosProvider::uiPosProvider( uiParent* p, const uiPosProvider::Setup& su )
	: uiGroup(p,"uiPosProvider")
	, setup_(su)
	, selfld_(0)
	, fullsurvbut_(0)
	, posProvGroupChanged(this)
{
    const BufferStringSet& factnms( setup_.is2d_
	    ? Pos::Provider2D::factory().getNames()
	    : Pos::Provider3D::factory().getNames() );

    const uiStringSet& factusrnms( setup_.is2d_
	   ? Pos::Provider2D::factory().getUserNames()
	   : Pos::Provider3D::factory().getUserNames() );

    uiStringSet nms;
    BufferStringSet reqnms;
    if ( setup_.choicetype_ != Setup::All )
    {
	reqnms.add( sKey::Range() );
	if ( setup_.choicetype_ == Setup::OnlySeisTypes )
	{
	    reqnms.add( sKey::Table() );
	    reqnms.add( sKey::Polygon() );
	    reqnms.add( "Seismic Cube Positions" );
				// In 7.x, sKey::SeisCubePositions()
	}
	else if ( setup_.choicetype_ == Setup::VolumeTypes )
	{
	    reqnms.add( sKey::Well() );
	    reqnms.add( sKey::Polygon() );
	    reqnms.add( sKey::Body() );
	}
	else if ( setup_.choicetype_ == Setup::RangewithPolygon )
	    reqnms.add( sKey::Polygon() );
    }

    for ( int idx=0; idx<factnms.size(); idx++ )
    {
	const OD::String& nm( factnms.get(idx) );
	if ( !reqnms.isEmpty() && !reqnms.isPresent(nm) )
	    continue;

	uiPosProvGroup* grp = uiPosProvGroup::factory()
				.create(nm,this,setup_,true);
	if ( !grp ) continue;

	nms.add( factusrnms[idx] );
	grp->setName( nm );
	grps_ += grp;

	mAttachCB(grp->posProvGroupChg, uiPosProvider::selChg);
    }
    if ( setup_.allownone_ )
	nms.add( uiStrings::sAll() );

    const CallBack selcb( mCB(this,uiPosProvider,selChg) );
    if ( grps_.size() == 0 )
    {
	new uiLabel( this, tr("No position providers available") );
	return;
    }

    uiObject* attachobj = 0;
    if ( nms.size() > 1 )
    {
	selfld_ = new uiGenInput( this, setup_.seltxt_, StringListInpSpec(nms));
	selfld_->valuechanged.notify( selcb );
	attachobj = selfld_->attachObj();
    }

    if ( !setup_.is2d_ )
    {
	fullsurvbut_ = new uiToolButton( this, "exttofullsurv",
			setup_.useworkarea_ ? tr("Set ranges to work area")
					    : tr("Set ranges to full survey"),
			mCB(this,uiPosProvider,fullSurvPush) );
	if ( selfld_ )
	    fullsurvbut_->attach( rightOf, selfld_ );
	else
	    attachobj = fullsurvbut_;
    }

    openbut_ = new uiToolButton( this, "open",
				tr("Open subselection"),
				mCB(this,uiPosProvider,openCB) );
    if ( fullsurvbut_ )
	openbut_->attach( rightTo, fullsurvbut_ );
    else if ( selfld_ )
	openbut_->attach( rightOf, selfld_ );
    else
	attachobj = openbut_;

    savebut_ = new uiToolButton( this, "save",
				tr("Save subselection"),
				mCB(this,uiPosProvider,saveCB) );
    savebut_->attach( rightTo, openbut_ );

    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->attach( alignedBelow, attachobj );

    setHAlignObj( grps_[0] );
    postFinalize().notify( selcb );
}


uiPosProvider::~uiPosProvider()
{
    detachAllNotifiers();
}


void uiPosProvider::selChg( CallBacker* )
{
    if ( !selfld_ ) return;
    const int selidx = selfld_->getIntValue();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == selidx );

    if ( fullsurvbut_ )
	fullsurvbut_->display( BufferString(selfld_->text()) == sKey::Range() );

    savebut_->setSensitive( grps_.validIdx(selidx) );

    posProvGroupChanged.trigger();
}


void uiPosProvider::fullSurvPush( CallBacker* )
{
    const int selidx = selfld_ ? selfld_->getIntValue() : 0;
    if ( selidx < 0 ) return;

    IOPar iop;
    SI().sampling( setup_.useworkarea_ ).fillPar( iop );
    grps_[selidx]->usePar( iop );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiPosProvider::openCB( CallBacker* )
{
    CtxtIOObj ctio( PosProvidersTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;
    ctio.fillDefault();
    uiIOObjSelDlg dlg( this, ctio, tr("Open Subselection") );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj_;
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open input file:\n%1").arg(fnm) )

    ascistream astrm( strm,true );
    IOPar iop( astrm );
    if ( iop.isEmpty() )
	mErrRet( tr("No valid subselection found") )

    usePar( iop );
    selChg(0);
}


void uiPosProvider::saveCB( CallBacker* )
{
    const int selidx = selfld_ ? selfld_->getIntValue() : 0;
    if ( !grps_.validIdx(selidx) )
	return;

    IOPar iop;
    grps_[selidx]->fillPar( iop );

    CtxtIOObj ctio( PosProvidersTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio, tr("Save Subselection") );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj_;
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open output file:\n%1").arg(fnm) )

    ascostream astrm( strm );
    if ( !astrm.putHeader("PosProvider") )
	mErrRet( tr("Cannot write to output file:\n%1").arg(fnm) )

    iop.putTo( astrm );
}


uiPosProvGroup* uiPosProvider::curGrp() const
{
    if ( grps_.size() < 1 ) return 0;
    const int selidx = selfld_ ? selfld_->getIntValue() : 0;
    return const_cast<uiPosProvGroup*>(
	    selidx < grps_.size() ? grps_[selidx] : 0);
}


void uiPosProvider::setSampling( const TrcKeyZSampling& tkzs )
{
    IOPar iop;
    iop.set( sKey::Type(), sKey::Range() );
    if ( setup_.is2d_ )
    {
	Pos::RangeProvider2D prov;
	prov.setTrcRange( tkzs.hsamp_.trcRange() );
	prov.setZRange( tkzs.zsamp_ );
	prov.fillPar( iop );
    }
    else
    {
	Pos::RangeProvider3D prov; prov.setSampling( tkzs );
	prov.fillPar( iop );
    }

    usePar( iop );
}


void uiPosProvider::getSampling( TrcKeyZSampling& tkzs,
				 const IOPar* pars ) const
{
    IOPar iop;
    if ( pars )
	iop = *pars;
    else
	fillPar( iop );

    PtrMan<Pos::Provider> prov = 0;
    if ( setup_.is2d_ )
	prov = Pos::Provider2D::make( iop );
    else
	prov = Pos::Provider3D::make( iop );

    if ( prov )
	prov->getTrcKeyZSampling( tkzs );
}


bool uiPosProvider::hasRandomSampling() const
{
    if ( !isAll() )
    {
	const uiPosProvGroup* curgrp = curGrp();
	if ( curgrp )
	    return curgrp->hasRandomSampling();
    }
    return false;
}

void uiPosProvider::usePar( const IOPar& iop )
{
    BufferString typ;
    iop.get( sKey::Type(), typ );
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( typ == grps_[idx]->name() )
	{
	    grps_[idx]->usePar( iop );
	    if ( selfld_ )
		selfld_->setValue( idx );
	    return;
	}
    }

    if ( selfld_ )
	selfld_->setValue( ((int)0) );

    if ( setup_.is2d_ )
	return;

// Provider from IOPar is not available in the gui.
// Perhaps we can still get a TrcKeyZSampling

    TrcKeyZSampling tkzs;
    getSampling( tkzs, &iop );
    setSampling( tkzs );
}


bool uiPosProvider::fillPar( IOPar& iop ) const
{
    if ( grps_.size() < 1 )
	return false;

    if ( !isAll() )
    {
	const uiPosProvGroup* curgrp = curGrp();
	return curgrp ? curgrp->fillPar(iop) : true;
    }

    iop.set( sKey::Type(), sKey::None() );
    return true;
}


void uiPosProvider::setExtractionDefaults()
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->setExtractionDefaults();
}


Pos::Provider* uiPosProvider::createProvider() const
{
    IOPar iop;
    if ( !fillPar(iop) )
	return 0;

    if ( setup_.is2d_ )
	return Pos::Provider2D::make( iop );
    else
	return Pos::Provider3D::make( iop );
}


uiPosProvSel::uiPosProvSel( uiParent* p, const uiPosProvSel::Setup& su )
    : uiCompoundParSel(p,su.seltxt_)
    , setup_(su)
    , prov_(0)
    , tkzs_(*new TrcKeyZSampling(false))
{
    txtfld_->setElemSzPol( uiObject::WideVar );
    iop_.set( sKey::Type(), sKey::None() );
    mkNewProv(false);
    butPush.notify( mCB(this,uiPosProvSel,doDlg) );
}


uiPosProvSel::~uiPosProvSel()
{
    delete prov_;
    delete &tkzs_;
}


BufferString uiPosProvSel::getSummary() const
{
    BufferString ret;
    if ( !prov_ )
	ret = "-";
    else
    {
	ret = prov_->type(); ret[1] = '\0'; ret += ": ";
	prov_->getSummary( ret );
    }
    return ret;
}


void uiPosProvSel::setCSToAll() const
{
    if ( setup_.is2d_ )
	tkzs_.set2DDef();
    else
	tkzs_ = SI().sampling( true );
}


void uiPosProvSel::setProvFromCS()
{
    delete prov_;
    if ( setup_.is2d_ )
    {
	Pos::RangeProvider2D* rp2d = new Pos::RangeProvider2D;
	rp2d->setTrcRange( tkzs_.hsamp_.crlRange(), 0 );
	rp2d->setZRange( tkzs_.zsamp_, 0 );
	prov_ = rp2d;
    }
    else
    {
	Pos::RangeProvider3D* rp3d = new Pos::RangeProvider3D;
	rp3d->setSampling( tkzs_ );
	prov_ = rp3d;
    }
    prov_->fillPar( iop_ );
    iop_.set( sKey::Type(), prov_->type() );
    updateSummary();
}


const TrcKeyZSampling& uiPosProvSel::envelope() const
{
    return tkzs_;
}


void uiPosProvSel::mkNewProv( bool updsumm )
{
    delete prov_;
    if ( setup_.is2d_ )
	prov_ = Pos::Provider2D::make( iop_ );
    else
	prov_ = Pos::Provider3D::make( iop_ );

    if ( prov_ )
    {
	prov_->getTrcKeyZSampling( tkzs_ );
	tkzs_.fillPar(iop_);
	if ( !setup_.is2d_ ) //set step for 3D cs
	{
	    TrcKeyZSampling tmpcs;
	    tmpcs.usePar( iop_ );
	    tkzs_.hsamp_.step_ = tmpcs.hsamp_.step_;
	}
    }
    else
    {
	setCSToAll();
	if ( !setup_.allownone_ )
	    { setProvFromCS(); return; }
    }

    if ( updsumm )
	updateSummary();
}


void uiPosProvSel::setInput( const TrcKeyZSampling& cs, bool chgtyp )
{
    if ( chgtyp || (prov_ && sKey::Range()==prov_->type()) )
    {
	tkzs_ = cs;
	setProvFromCS();
    }
}


void uiPosProvSel::setInput( const TrcKeyZSampling& initcs,
			     const TrcKeyZSampling& ioparcs )
{
    setInput( initcs );
    ioparcs.fillPar( iop_ );
}


void uiPosProvSel::setInputLimit( const TrcKeyZSampling& cs )
{
    setup_.tkzs_ = cs;
}


bool uiPosProvSel::isAll() const
{
    if ( setup_.allownone_ )
	return !prov_;

    TrcKeyZSampling cskp = tkzs_;
    setCSToAll();
    const bool ret = tkzs_ == cskp;
    tkzs_ = cskp;
    return ret;
}


void uiPosProvSel::setToAll()
{
    if ( !setup_.allownone_ )
    {
	iop_.set( sKey::Type(), sKey::None() );
	mkNewProv( true );
    }
    else
    {
	setCSToAll();
	setProvFromCS();
    }
}


void uiPosProvSel::doDlg( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(uiStrings::sPosition(mPlural),
			   uiStrings::phrSpecify(uiStrings::sPosition(mPlural)),
			   mODHelpKey(mPosProvSelHelpID) ) );
    uiPosProvider* pp = new uiPosProvider( &dlg, setup_ );
    pp->usePar( iop_ );
    if ( dlg.go() )
    {
	iop_.setEmpty();
	pp->fillPar( iop_ );
	mkNewProv();
    }
}


void uiPosProvSel::usePar( const IOPar& iop )
{
    IOPar orgiop( iop_ );
    iop_.merge( iop );
    mkNewProv();
    iop_ = orgiop;
    if ( prov_ )
    {
	prov_->fillPar( iop_ );
	iop_.set(sKey::Type(),prov_->type());
    }
}



// uiPosProvDlg
uiPosProvDlg::uiPosProvDlg( uiParent* p, const Setup& su, const uiString& title)
    : uiDialog(p,uiDialog::Setup(title,mNoDlgTitle,mNoHelpKey))
{
    selfld_ = new uiPosProvider( this, su );
    selfld_->setSampling( SI().sampling(su.useworkarea_) );
}


uiPosProvDlg::~uiPosProvDlg()
{}

void uiPosProvDlg::setSampling( const TrcKeyZSampling& tkzs )
{ selfld_->setSampling( tkzs ); }

void uiPosProvDlg::getSampling( TrcKeyZSampling& tkzs, const IOPar* pars ) const
{ selfld_->getSampling( tkzs, pars ); }


bool uiPosProvDlg::acceptOK( CallBacker* )
{ return true; }


// uiPosSubSel

uiPosSubSel::uiPosSubSel( uiParent* p, const uiPosSubSel::Setup& su )
    : uiGroup(p,"uiPosSubSel")
    , selChange(this)
{
    uiPosProvider::Setup ppsu( su.is2d_, su.withstep_, su.withz_ );
    ppsu.seltxt( mToUiStringTodo(su.seltxt_) )
	.allownone( true )
	.choicetype( (uiPosProvider::Setup::ChoiceType)su.choicetype_ );
    ppsu.zdomkey( su.zdomkey_ );
    ps_ = new uiPosProvSel( this, ppsu );
    ps_->butPush.notify( mCB(this,uiPosSubSel,selChg) );
    setHAlignObj( ps_ );
}


uiPosSubSel::~uiPosSubSel()
{}


void uiPosSubSel::selChg( CallBacker* )
{
    selChange.trigger();
}


#define mDefFn(ret,nm,typ,arg,cnst,retstmt) \
ret uiPosSubSel::nm( typ arg ) cnst \
{ \
    retstmt ps_->nm( arg ); \
}
#define mDefFn2(ret,nm,typ1,arg1,typ2,arg2,cnst) \
ret uiPosSubSel::nm( typ1 arg1, typ2 arg2 ) cnst \
{ \
    ps_->nm( arg1, arg2 ); \
    selChg(0); \
}

mDefFn(void,usePar,const IOPar&,iop,,)
mDefFn(void,fillPar,IOPar&,iop,const,)
mDefFn(Pos::Provider*,curProvider,,,,return)
mDefFn(const Pos::Provider*,curProvider,,,const,return)
mDefFn(const TrcKeyZSampling&,envelope,,,const,return)
mDefFn(const TrcKeyZSampling&,inputLimit,,,const,return)
mDefFn(bool,isAll,,,const,return)
mDefFn(void,setToAll,,,,)
mDefFn(void,setInputLimit,const TrcKeyZSampling&,cs,,)
mDefFn2(void,setInput,const TrcKeyZSampling&,cs,bool,ct,)
mDefFn2(void,setInput,const TrcKeyZSampling&,initcs,const TrcKeyZSampling&,
	ioparcs,)
