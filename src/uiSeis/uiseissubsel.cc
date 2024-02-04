/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseissubsel.h"

#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posprovider.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "zdomain.h"

#include "uicompoundparsel.h"
#include "uimsg.h"
#include "uiposprovider.h"
#include "uipossubsel.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"


// uiSeisSubSel

uiSeisSubSel* uiSeisSubSel::get( uiParent* p, const Seis::SelSetup& ss )
{
    if ( ss.is2d_ )
       return new uiSeis2DSubSel( p, ss );
    else
       return new uiSeis3DSubSel( p, ss );
}


uiSeisSubSel::uiSeisSubSel( uiParent* p, const Seis::SelSetup& ss )
    : uiGroup(p,"Seis subsel")
    , selChange(this)
{
    uiPosSubSel::Setup pss( ss.is2d_, !ss.withoutz_ );
    pss.withstep(ss.withstep_)
       .choicetype(ss.onlyrange_ ? uiPosSubSel::Setup::OnlyRanges
				 : uiPosSubSel::Setup::OnlySeisTypes)
       .zdomkey( ss.zdomkey_ ).zunitstr( ss.zunitstr_ );

    selfld_ = new uiPosSubSel( this, pss );
    mAttachCB( selfld_->selChange, uiSeisSubSel::selChangeCB );
    setHAlignObj( selfld_ );

    mAttachCB( IOM().afterSurveyChange, uiSeisSubSel::afterSurveyChangedCB );
}


uiSeisSubSel::~uiSeisSubSel()
{
    detachAllNotifiers();
}


void uiSeisSubSel::selChangeCB( CallBacker* )
{ selChange.trigger(); }


void uiSeisSubSel::afterSurveyChangedCB( CallBacker* )
{
    if ( IOM().isBad() )
	return;

    selfld_->setInputLimit( SI().sampling(true) );
}


bool uiSeisSubSel::isAll() const
{
    return selfld_->isAll();
}


void uiSeisSubSel::getSampling( TrcKeyZSampling& cs ) const
{
    cs = selfld_->envelope();
}


void uiSeisSubSel::getSampling( TrcKeySampling& hs ) const
{
    hs = selfld_->envelope().hsamp_;
}


void uiSeisSubSel::getZRange( StepInterval<float>& zrg ) const
{
    zrg = selfld_->envelope().zsamp_;
}


const ZDomain::Info* uiSeisSubSel::zDomain() const
{
    return selfld_->zDomain();
}


bool uiSeisSubSel::fillPar( IOPar& iop ) const
{
    selfld_->fillPar(iop); return true;
}


void uiSeisSubSel::usePar( const IOPar& iop )
{
    selfld_->usePar( iop );
}


void uiSeisSubSel::clear()
{
    selfld_->clear();
}


void uiSeisSubSel::setInput( const MultiID& id )
{
    if ( id.isUdf() )
	return;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( ioobj )
	setInput( *ioobj );
}


void uiSeisSubSel::setInput( const TrcKeySampling& tks )
{
    TrcKeyZSampling tkzs = selfld_->envelope();
    tkzs.hsamp_ = tks;
    selfld_->setInput( tkzs );
}


void uiSeisSubSel::setInput( const StepInterval<float>& zrg )
{
    TrcKeyZSampling tkzs = selfld_->envelope();
    tkzs.zsamp_ = zrg;
    selfld_->setInput( tkzs );
}


void uiSeisSubSel::setInput( const TrcKeyZSampling& tkzs )
{
    selfld_->setInput( tkzs );
}


void uiSeisSubSel::setInputLimit( const TrcKeyZSampling& tkzs )
{
    selfld_->setInputLimit( tkzs );
}


int uiSeisSubSel::expectedNrSamples() const
{
    const Pos::Provider* pp = selfld_->curProvider();
    return pp ? pp->estNrZPerPos() : 0;
}


int uiSeisSubSel::expectedNrTraces() const
{
    const Pos::Provider* pp = selfld_->curProvider();
    return pp ? sCast(int,pp->estNrPos()) : 0;
}


uiCompoundParSel* uiSeisSubSel::compoundParSel()
{
    return selfld_->provSel();
}



// uiSeis3DSubSel

uiSeis3DSubSel::uiSeis3DSubSel( uiParent* p, const Seis::SelSetup& ss )
    : uiSeisSubSel(p,ss)
{}


uiSeis3DSubSel::~uiSeis3DSubSel()
{}


void uiSeis3DSubSel::setInput( const IOObj& ioobj )
{
    const SeisIOObjInfo oinf( ioobj );
    if ( !oinf.isOK() )
    {
	clear();
	return;
    }

    TrcKeyZSampling tkzs;
    if ( !oinf.getRanges(tkzs) )
    {
	clear();
	return;
    }

    if ( !oinf.zDomain().isCompatibleWith(SI().zDomainInfo()) )
	selfld_->setInputLimit( tkzs );

    selfld_->setInput( tkzs );
}



// uiSeis2DSubSel

uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, const Seis::SelSetup& ss )
    : uiSeisSubSel(p,ss)
{
    if ( ss.multiline_ )
    {
	uiSeis2DMultiLineSel::Setup su( ss.seltxt_, true, false,
					!ss.withoutz_, ss.withstep_ );
	su.zdomkey( ss.zdomkey_ ).zunitstr( ss.zunitstr_ );
	multilnmsel_ = new uiSeis2DMultiLineSel( this, su );
	setHAlignObj( multilnmsel_ );
	mAttachCB( multilnmsel_->selectionChanged, uiSeis2DSubSel::lineChg );
    }
    else
    {
	singlelnmsel_ = new uiSeis2DLineNameSel( this, !ss.fornewentry_ );
	setHAlignObj( singlelnmsel_ );
	mAttachCB( singlelnmsel_->nameChanged, uiSeis2DSubSel::lineChg );
    }

    if ( multilnmsel_ )
	selfld_->display( false );
    else
	selfld_->attach( alignedBelow, singlelnmsel_ );

}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
    detachAllNotifiers();
}


void uiSeis2DSubSel::clear()
{
    uiSeisSubSel::clear();
    if ( multilnmsel_ )
	multilnmsel_->clearSelection();
}


void uiSeis2DSubSel::setInput( const IOObj& ioobj )
{
    clear();
    inpkey_ = ioobj.key();
    if ( multilnmsel_ )
	multilnmsel_->setInput( inpkey_ );
    else
	singlelnmsel_->setDataSet( inpkey_ );
}


void uiSeis2DSubSel::setInputLines( const TypeSet<Pos::GeomID>& geomids )
{
    if ( multilnmsel_ )
	multilnmsel_->setInput( geomids );
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    BufferStringSet lnms;
    iopar.get( sKey::LineKey(), lnms );
    if ( lnms.isEmpty() && multilnmsel_ )
    {
	multilnmsel_->usePar( iopar );
	return;
    }

    if ( multilnmsel_ )
	multilnmsel_->setSelLineNames( lnms );
    else if ( !lnms.isEmpty() )
	singlelnmsel_->setInput( lnms.get(0).buf() );
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    if ( multilnmsel_ )
    {
	if ( !multilnmsel_->nrSelected() )
	{
	    uiMSG().error( tr("Please select at least one line") );
	    return false;
	}

	return multilnmsel_->fillPar( iopar );
    }

    const StringView sellinenm( singlelnmsel_->getInput() );
    if ( sellinenm.isEmpty() )
    {
	uiMSG().error( tr("Please select the line") );
	return false;
    }

    iopar.set( sKey::LineKey(), sellinenm );
    iopar.set( sKey::GeomID(), singlelnmsel_->getInputGeomID() );
    return uiSeisSubSel::fillPar( iopar );
}


bool uiSeis2DSubSel::isSingLine() const
{
    return !multilnmsel_ || (multilnmsel_->nrSelected() == 1);
}


BufferString uiSeis2DSubSel::selectedLine() const
{
    return multilnmsel_ ? multilnmsel_->lineName() : singlelnmsel_->getInput();
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    if ( multilnmsel_ )
	multilnmsel_->setSelLine( nm );
    else
	singlelnmsel_->setInput( nm );
}


void uiSeis2DSubSel::selectedGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    geomids.erase();
    if ( multilnmsel_ )
	multilnmsel_->getSelGeomIDs( geomids );
    else
	geomids += singlelnmsel_->getInputGeomID();
}


void uiSeis2DSubSel::selectedLines( BufferStringSet& lnms ) const
{
    lnms.erase();
    if ( multilnmsel_ )
	multilnmsel_->getSelLineNames( lnms );
    else
	lnms.add( singlelnmsel_->getInput() );
}


void uiSeis2DSubSel::setSelectedLines( const BufferStringSet& lnms )
{
    if ( multilnmsel_ )
	multilnmsel_->setSelLineNames( lnms );
    else if ( !lnms.isEmpty() )
	singlelnmsel_->setInput( lnms.get(0) );
}


int uiSeis2DSubSel::expectedNrSamples() const
{
    TypeSet<Pos::GeomID> geomids;
    selectedGeomIDs( geomids );
    const Pos::GeomID geomid = geomids.isEmpty() ? Pos::GeomID::udf()
						 : geomids.first();
    return getZRange(geomid).nrSteps() + 1;
}


int uiSeis2DSubSel::expectedNrTraces() const
{
    int totalnrtraces = 0;
    TypeSet<Pos::GeomID> geomids;
    selectedGeomIDs( geomids );
    for ( int idx=0; idx<geomids.size(); idx++ )
	totalnrtraces += getTrcRange(geomids[idx]).nrSteps()+1;

    return totalnrtraces;
}


StepInterval<int> uiSeis2DSubSel::getTrcRange( Pos::GeomID geomid ) const
{
    StepInterval<int> trcrg = StepInterval<int>::udf();
    if ( multilnmsel_ )
	trcrg = multilnmsel_->getTrcRange( geomid );
    else
	trcrg = selfld_->envelope().hsamp_.crlRange();

    return trcrg;
}


StepInterval<float> uiSeis2DSubSel::getZRange( Pos::GeomID geomid ) const
{
    StepInterval<float> zrg = StepInterval<float>::udf();
    if ( multilnmsel_ )
	zrg = multilnmsel_->getZRange( geomid );
    else
	zrg = selfld_->envelope().zsamp_;

    return zrg;

}


void uiSeis2DSubSel::getSampling( TrcKeyZSampling& tkzs,
				  Pos::GeomID geomid ) const
{
    if ( singlelnmsel_ && geomid.isUdf() )
	geomid = singlelnmsel_->getInputGeomID();

    tkzs.set2DDef();
    tkzs.hsamp_.setGeomID( geomid );
    tkzs.hsamp_.setTrcRange( getTrcRange(geomid) );
    tkzs.zsamp_ = getZRange( geomid );

}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    if ( singlelnmsel_ )
    {
	const Pos::GeomID selid = singlelnmsel_->getInputGeomID();
	SeisIOObjInfo oif( inpkey_ );
	StepInterval<float> zrg;
	StepInterval<int> trcrg;
	if ( oif.getRanges(selid,trcrg,zrg) )
	{
	    TrcKeyZSampling cs;
	    StepInterval<int> inlrg( 0, 0, 1 );
	    cs.hsamp_.set( inlrg, trcrg );
	    cs.zsamp_ = zrg;

	    const bool hasworkzrg = SI().zRange(true) != SI().zRange(false);
	    if ( hasworkzrg )
		cs.zsamp_.limitTo( SI().zRange(true) );

	    selfld_->setInput( cs );
	    selfld_->setInputLimit( cs );
	}
    }

    selChange.trigger();
}



// uiMultiZSeisSubSel

uiMultiZSeisSubSel::uiMultiZSeisSubSel( uiParent* p, const Seis::SelSetup& ss,
			    const ObjectSet<const ZDomain::Info>* zinfos_in )
    : uiGroup(p)
    , selChange(this)
{
    ObjectSet<const ZDomain::Info> zinfos;
    if ( zinfos_in )
	zinfos = *zinfos_in;
    else
	zinfos = ZDomain::sAll();

    const ZDomain::Info& siinfo = SI().zDomainInfo();
    if ( zinfos.isEmpty() )
    {
	pErrMsg("Should not happen");
	zinfos.add( &siinfo );
    }

    if ( zinfos.isPresent(&siinfo) )
    { //Put SI first, only to speed up the searches (most frequent case)
	zinfos -= &siinfo;
	zinfos.insertAt( &siinfo, 0 );
    }

    for ( const auto* zinfo : zinfos )
    {
	if ( !zinfo )
	    continue;

	Seis::SelSetup seisss( ss );
	seisss.zdomkey( zinfo->key() ).zunitstr( zinfo->unitStr() );
	auto* subselfld = uiSeisSubSel::get( this, seisss );
	mAttachCB( subselfld->selChange, uiMultiZSeisSubSel::selCB );
	subselflds_.add( subselfld );
    }

    if ( subselflds_.isEmpty() )
	return;

    setHAlignObj( subselflds_.first()->attachObj() );
    mAttachCB( postFinalize(), uiMultiZSeisSubSel::initGrpCB );
}


uiMultiZSeisSubSel::~uiMultiZSeisSubSel()
{
    detachAllNotifiers();
}


void uiMultiZSeisSubSel::initGrpCB( CallBacker* )
{
    if ( !setZDomain(SI().zDomainInfo()) && !subselflds_.isEmpty() )
    {
	for ( auto* subselfld : subselflds_ )
	    subselfld->display( false );

	subselflds_.first()->display( true );
    }
}


void uiMultiZSeisSubSel::selCB( CallBacker* )
{
    selChange.trigger( getSelGrp() );
}


bool uiMultiZSeisSubSel::setInput( const MultiID& id )
{
    if ( id.isUdf() )
	return false;

    PtrMan<IOObj> ioobj = IOM().get( id );
    return ioobj ? setInput( *ioobj ) : false;
}


bool uiMultiZSeisSubSel::setInput( const IOObj& ioobj )
{
    const SeisIOObjInfo info( ioobj );
    if ( !info.isOK() )
	return false;

    if ( !setZDomain(info.zDomain()) )
	return false;

    uiSeisSubSel* subselfld = getSelGrp();
    if ( !subselfld )
	return false;

    NotifyStopper ns( subselfld->selChange );
    subselfld->setInput( ioobj );
    selChange.trigger();
    return true;
}


bool uiMultiZSeisSubSel::setZDomain( const ZDomain::Info& zinfo )
{
    bool isfound = false;
    for ( auto* subselfld : subselflds_ )
    {
	const ZDomain::Info* zinf = subselfld->zDomain();
	if ( zinf && *zinf == zinfo )
	{
	    subselfld->display( true );
	    isfound = true;
	}
	else
	    subselfld->display( false );
    }

    return isfound;
}


bool uiMultiZSeisSubSel::setInput( const TrcKeyZSampling& tkzs,
				   const ZDomain::Info* zinfo )
{
    uiSeisSubSel* subselfld = getSelGrp( zinfo );
    if ( !subselfld )
	return false;

    subselfld->setInput( tkzs );
    return true;
}


bool uiMultiZSeisSubSel::setInputLimit( const TrcKeyZSampling& tkzs,
					const ZDomain::Info* zinfo )
{
    uiSeisSubSel* subselfld = getSelGrp( zinfo );
    if ( !subselfld )
	return false;

    subselfld->setInputLimit( tkzs );
    return true;
}


bool uiMultiZSeisSubSel::getSampling( TrcKeyZSampling& tkzs ) const
{
    const uiSeisSubSel* subselfld = getSelGrp();
    if ( !subselfld )
	return false;

    subselfld->getSampling( tkzs );
    return true;
}


const ZDomain::Info* uiMultiZSeisSubSel::zDomain() const
{
    const uiSeisSubSel* subselfld = getSelGrp();
    return subselfld ? subselfld->zDomain() : nullptr;
}


const uiSeisSubSel* uiMultiZSeisSubSel::getSelGrp() const
{
    return mSelf().getSelGrp();
}


uiSeisSubSel* uiMultiZSeisSubSel::getSelGrp()
{
    for ( auto* subselfld : subselflds_ )
    {
	if ( subselfld->isDisplayed() )
	    return subselfld;
    }

    return nullptr;
}


const uiSeisSubSel* uiMultiZSeisSubSel::getSelGrp(
					const ZDomain::Info* zinfo ) const
{
    return mSelf().getSelGrp( zinfo );
}


uiSeisSubSel* uiMultiZSeisSubSel::getSelGrp( const ZDomain::Info* zinfo )
{
    if ( zinfo )
    {
	for ( auto* subselfld : subselflds_ )
	{
	    const ZDomain::Info* zinf = subselfld->zDomain();
	    if ( zinf && *zinf == *zinfo )
		return subselfld;
	}
    }

    return getSelGrp();
}
