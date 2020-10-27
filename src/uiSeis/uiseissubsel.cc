/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2004
________________________________________________________________________

-*/

#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uicompoundparsel.h"
#include "uipossubsel.h"
#include "uiposprovider.h"
#include "uibutton.h"
#include "uilistbox.h"
#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seisselsetup.h"
#include "survinfo.h"
#include "iopar.h"
#include "ioobj.h"
#include "dbman.h"
#include "linesubsel.h"
#include "trckeyzsampling.h"
#include "keystrs.h"
#include "posprovider.h"
#include "zdomain.h"


uiSeisSubSel* uiSeisSubSel::get( uiParent* p, const Seis::SelSetup& s )
{
    if ( s.is2d_ )
       return new uiSeis2DSubSel( p, s );
    else
       return new uiSeis3DSubSel( p, s );
}


uiSeisSubSel::uiSeisSubSel( uiParent* p, const Seis::SelSetup& ss )
    : uiGroup(p,"Seis subsel")
    , selChange(this)
{
    uiPosSubSel::Setup pss( ss.is2d_, !ss.withoutz_ );
    pss.withstep(ss.withstep_)
	.choicetype(ss.onlyrange_ ? uiPosSubSel::Setup::OnlyRanges
				  : uiPosSubSel::Setup::OnlySeisTypes)
	.zdomkey(ss.zdomkey_);

    selfld_ = new uiPosSubSel( this, pss );
    selfld_->selChange.notify( mCB(this,uiSeisSubSel,selChangeCB) );
    setHAlignObj( selfld_ );

    mAttachCB( DBM().afterSurveyChange, uiSeisSubSel::afterSurveyChangedCB);
}


uiSeisSubSel::~uiSeisSubSel()
{
    detachAllNotifiers();
}


void uiSeisSubSel::selChangeCB( CallBacker* )
{ selChange.trigger(); }


void uiSeisSubSel::afterSurveyChangedCB( CallBacker* )
{
    if ( DBM().isBad() )
	return;

    TrcKeyZSampling tkzs( OD::UsrWork );
    selfld_->setInputLimit( tkzs );
}


void uiSeisSubSel::setSurvey( const SurveyInfo& si )
{
    TrcKeyZSampling tkzs( false );
    si.getSampling( tkzs );
    selfld_->setInputLimit( tkzs );
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


void uiSeisSubSel::setInput( const DBKey& id )
{
    IOObj* ioobj = id.getIOObj();
    if ( ioobj )
	setInput( *ioobj );
    delete ioobj;
}


void uiSeisSubSel::setInput( const TrcKeySampling& hs )
{
    TrcKeyZSampling cs = selfld_->envelope(); cs.hsamp_ = hs;
    selfld_->setInput( cs );
}


void uiSeisSubSel::setInput( const StepInterval<float>& zrg )
{
    TrcKeyZSampling cs = selfld_->envelope(); cs.zsamp_ = zrg;
    selfld_->setInput( cs );
}


void uiSeisSubSel::setInput( const TrcKeyZSampling& cs )
{
    selfld_->setInput( cs );
}


int uiSeisSubSel::expectedNrSamples() const
{
    const Pos::Provider* pp = selfld_->curProvider();
    return pp ? pp->estNrZPerPos() : 0;
}


int uiSeisSubSel::expectedNrTraces() const
{
    const Pos::Provider* pp = selfld_->curProvider();
    return pp ? mCast(int,pp->estNrPos()) : 0;
}


uiCompoundParSel* uiSeisSubSel::compoundParSel()
{
    return selfld_->provSel();
}


void uiSeis3DSubSel::setInput( const IOObj& ioobj )
{
    SeisIOObjInfo oinf( ioobj ); TrcKeyZSampling cs;
    if ( !oinf.getRanges(cs) )
	clear();
    else
    {
	selfld_->setInput( cs );
	if ( &oinf.zDomainDef() != &ZDomain::SI() )
	{
	    TrcKeyZSampling limcs( selfld_->inputLimit() );
	    limcs.zsamp_.start = -1e9; limcs.zsamp_.stop = 1e9;
	    limcs.zsamp_.step = 0.001;
	    selfld_->setInputLimit( limcs );
	}
    }
}


uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, const Seis::SelSetup& ss )
	: uiSeisSubSel(p,ss)
	, multiln_(ss.multiline_)
	, multilnmsel_(0)
	, singlelnmsel_(0)
{
    if ( multiln_ )
    {
	multilnmsel_ = new uiSeis2DMultiLineSel(this, uiString::empty(),
						!ss.withoutz_, ss.withstep_ );
	setHAlignObj( multilnmsel_ );
	multilnmsel_->selectionChanged.notify(mCB(this,uiSeis2DSubSel,lineChg));
	selfld_->setToAll();
    }
    else
    {
	singlelnmsel_ = new uiSeis2DLineNameSel( this, !ss.fornewentry_ );
	setHAlignObj( singlelnmsel_ );
	singlelnmsel_->nameChanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
    }

    if ( multilnmsel_ )
	selfld_->display( false );
    else
	selfld_->attach( alignedBelow, singlelnmsel_ );

}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
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


void uiSeis2DSubSel::setInputLines( const GeomIDSet& geomids )
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
	    uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(
					uiStrings::sLine() ) );
	    return false;
	}

	return multilnmsel_->fillPar( iopar );
    }

    const FixedString sellinenm( singlelnmsel_->getInput() );
    if ( sellinenm.isEmpty() )
    {
	uiMSG().error( uiStrings::phrSelect(tr("the line")) );
	return false;
    }

    iopar.set( sKey::LineKey(), sellinenm );
    return uiSeisSubSel::fillPar( iopar );
}


bool uiSeis2DSubSel::isSingLine() const
{
    return !multilnmsel_ || (multilnmsel_->nrSelected() == 1);
}


const char* uiSeis2DSubSel::selectedLine() const
{
    return multilnmsel_ ? multilnmsel_->lineName() : singlelnmsel_->getInput();
}


Pos::GeomID uiSeis2DSubSel::selectedGeomID() const
{
    return multilnmsel_ ? multilnmsel_->geomID()
			: singlelnmsel_->getInputGeomID();
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    if ( multilnmsel_ )
	multilnmsel_->setSelLine( nm );
    else
    {
	singlelnmsel_->setInput( nm );
	if ( singlelnmsel_->getInputGeomID().isValid() )
	    selfld_->setSensitive( true );
	else
	{
	    selfld_->setToAll();
	    selfld_->setSensitive( false );
	}
    }
}


void uiSeis2DSubSel::selectedGeomIDs( GeomIDSet& geomids ) const
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
    return getZRange().nrSteps() + 1;
}


int uiSeis2DSubSel::expectedNrTraces() const
{
    int totalnrtraces = 0;
    GeomIDSet geomids;
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
	zrg.setFrom( selfld_->envelope().zsamp_ );

    return zrg;

}


void uiSeis2DSubSel::getSampling( TrcKeyZSampling& tkzs ) const
{
    if ( !singlelnmsel_ )
	uiSeisSubSel::getSampling( tkzs );
    else
	getTKZS( tkzs );
}


void uiSeis2DSubSel::getSampling( TrcKeySampling& tks ) const
{
    if ( !singlelnmsel_ )
	uiSeisSubSel::getSampling( tks );
    else
    {
	TrcKeyZSampling tkzs( tks );
	getTKZS( tkzs );
	tks = tkzs.hsamp_;
    }
}


void uiSeis2DSubSel::getTKZS( TrcKeyZSampling& tkzs, Pos::GeomID geomid ) const
{
    if ( singlelnmsel_ && !geomid.isValid() )
	geomid = singlelnmsel_->getInputGeomID();

    LineSubSel lss( geomid );
    lss.setTrcNrRange( getTrcRange(geomid) );
    lss.setZRange( getZRange(geomid) );
    tkzs = TrcKeyZSampling( lss );
}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    if ( singlelnmsel_ )
    {
	const GeomID geomid = singlelnmsel_->getInputGeomID();
	if ( geomid.isValid() )
	{
	    const LineSubSel lss( singlelnmsel_->getInputGeomID() );
	    const TrcKeyZSampling tkzs( lss );
	    selfld_->setInputLimit( tkzs );
	    selfld_->setInput( tkzs );
	    selfld_->setSensitive( true );
	}
	else
	{
	    selfld_->setToAll();
	    selfld_->setSensitive( false );
	}
    }

    selChange.trigger();
}
