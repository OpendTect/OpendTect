/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicreatepicks.h"

#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipointsetsel.h"
#include "uiposfilterset.h"
#include "uiposprovider.h"
#include "uitaskrunner.h"

#include "color.h"
#include "datapointset.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "pickset.h"
#include "picksettr.h"
#include "posprovider.h"
#include "randcolor.h"
#include "survinfo.h"


static int defnrpicks = 500;
static const char* sGeoms2D[] =
{
    "Z Range",
    "On Horizon",
    "Between Horizons",
    nullptr
};


// RandLocGenPars
RandLocGenPars::RandLocGenPars()
{}


RandLocGenPars::~RandLocGenPars()
{}


// uiCreatePicks
uiCreatePicks::uiCreatePicks( uiParent* p, bool aspoly, bool addstdflds,
			      bool zvalreq )
    : uiDialog(p,Setup(
	aspoly ? uiStrings::phrCreateNew(uiStrings::sPolygon()).toTitleCase()
	       : uiStrings::phrCreateNew(uiStrings::sPointSet()).toTitleCase(),
	mODHelpKey(mFetchPicksHelpID)))
    , picksetReady(this)
    , aspolygon_(aspoly)
    , iszvalreq_(zvalreq)
{
    setOkCancelText( uiStrings::sCreate(), uiStrings::sClose() );
    if ( addstdflds )
	addStdFields( nullptr );
}


uiCreatePicks::~uiCreatePicks()
{}


void uiCreatePicks::addStdFields( uiObject* lastobject )
{
    outfld_ = uiPointSetPolygonSel::create( this, false, aspolygon_ );

    colsel_ = new uiColorInput( this,
			      uiColorInput::Setup(OD::getRandStdDrawColor()).
			      lbltxt(uiStrings::sColor()) );
    colsel_->attach( alignedBelow, outfld_ );
    if ( lastobject )
	outfld_->attach( alignedBelow, lastobject );

    if ( iszvalreq_ )
    {
        float zval = SI().zRange(true).start_;
	zval *= SI().zDomain().userFactor();
	const uiString lbl(
		tr("Z value for Points %1").arg(SI().getUiZUnitString()) );
	zvalfld_ = new uiGenInput( this, lbl, FloatInpSpec(zval) );
	zvalfld_->attach( alignedBelow, colsel_ );
    }
}


RefMan<Pick::Set> uiCreatePicks::getPickSet() const
{
    RefMan<Pick::Set> ret = new Pick::Set( name_ );
    ret->disp_.color_ = colsel_->color();
    ret->disp_.connect_ = aspolygon_ ? Pick::Set::Disp::Open
				     : Pick::Set::Disp::None;
    return ret;
}


MultiID uiCreatePicks::getStoredID() const
{
    return outfld_ ? outfld_->key(true) : MultiID::udf();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiCreatePicks::acceptOK( CallBacker* )
{
    if ( !doAccept() || !handlePickSet() )
	return false;

    return isModal();
}


bool uiCreatePicks::doAccept()
{
    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	return false;

    name_ = ioobj->name();
    if ( iszvalreq_ && !calcZValAccToSurvDepth() )
	return false;

    return true;
}


bool uiCreatePicks::handlePickSet()
{
    RefMan<Pick::Set> ps = getPickSet();
    if ( !ps )
	return false;

    const IOObj* ioobj = outfld_->ioobj( true );
    IOM().commitChanges( *ioobj );
    uiString errmsg;
    if ( !PickSetTranslator::store(*ps,ioobj,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    const MultiID key = ioobj->key();
    Pick::SetMgr& psmgr = Pick::Mgr();
    int setidx = psmgr.indexOf( key );
    if ( setidx<0 )
    {
	Pick::Set* newps = new Pick::Set( *ps );
	psmgr.set( key, newps );
	setidx = psmgr.indexOf( key );
    }
    else
    {
	RefMan<Pick::Set> oldps = psmgr.get( setidx );
	oldps->setEmpty();
	oldps->append( *ps );
	oldps->disp_.color_ = ps->disp_.color_;
	psmgr.reportChange( nullptr, *oldps );
	psmgr.reportDispChange( nullptr, *oldps );
    }

    psmgr.setUnChanged( setidx, true );
    picksetReady.trigger();
    return true;
}


bool uiCreatePicks::calcZValAccToSurvDepth()
{
    const float zval = zvalfld_->getFValue();
    zval_ = zval / SI().zDomain().userFactor();

    StepInterval<float> zrg = SI().zRange(false);
    if ( SI().zDomain().isTime() )
	zrg.scale( (float)(SI().zDomain().userFactor()) );
    if ( zrg.includes(zval,true) )
	return true;

    uiMSG().error( tr("Input Z Value lies outside the survey range.\n"
		      "Survey range is: %1-%2 %3")
		    .arg(zrg.start_).arg(zrg.stop_).arg(SI().zDomain()
		    .uiUnitStr()));

    return false;
}



// uiGenPosPicks
uiGenPosPicks::uiGenPosPicks( uiParent* p )
    : uiCreatePicks(p,false,false)
    , posprovfld_(nullptr)
{
    uiPosProvider::Setup psu( false, true, true );
    psu.seltxt( tr("Generate locations by") )
	.choicetype( uiPosProvider::Setup::All ).withrandom(true);
    posprovfld_ = new uiPosProvider( this, psu );
    posprovfld_->setExtractionDefaults();

    uiPosFilterSet::Setup fsu( false );
    fsu.seltxt( uiStrings::phrRemove(tr("locations")) ).incprovs( true );
    posfiltfld_ = new uiPosFilterSetSel( this, fsu );
    posfiltfld_->attach( alignedBelow, posprovfld_ );

    addStdFields( posfiltfld_->attachObj() );
}


uiGenPosPicks::~uiGenPosPicks()
{
}


#define mSetCursor() MouseCursorManager::setOverride( MouseCursor::Wait )
#define mRestorCursor() MouseCursorManager::restoreOverride()

bool uiGenPosPicks::acceptOK( CallBacker* )
{
    if ( !posprovfld_ )
	return true;

    if ( !doAccept() )
	return false;

    PtrMan<Pos::Provider> prov = posprovfld_->createProvider();
    if ( !prov )
	mErrRet(::toUiString("Internal: no Pos::Provider"))

    uiTaskRunner taskrunner( this );
    if ( !prov->initialize( &taskrunner ) )
	return false;

    mSetCursor();
    IOPar iop; posfiltfld_->fillPar( iop );
    PtrMan<Pos::Filter> filt = Pos::Filter::make( iop, prov->is2D() );
    if ( filt && !filt->initialize(&taskrunner) )
	{ mRestorCursor(); return false; }

    dps_ = new DataPointSet( prov->is2D() );
    if ( !dps_->extractPositions(*prov,ObjectSet<DataColDef>(),filt.ptr(),
				 &taskrunner, true) )
    {
	dps_ = nullptr;
	return false;
    }

    mRestorCursor();

    const int dpssize = dps_->size();
    int size = dpssize;

    if ( size>50000 )
    {
	uiString msg = tr("PointSet would contain %1 points "
			  "which might consume unexpected time and memory."
			  "\n\nDo you want to continue?")
		     .arg(dpssize);
	if ( !uiMSG().askGoOn(msg) )
	{
	    mRestorCursor();
	    dps_ = nullptr;
	    return false;
	}
    }

    if ( dps_->isEmpty() )
    {
	dps_ = nullptr;
	mErrRet(tr("No matching locations found"))
    }

    const bool res = handlePickSet();
    dps_ = nullptr;
    return res ? isModal() : false;
}


RefMan<Pick::Set> uiGenPosPicks::getPickSet() const
{
    if ( !dps_ || dps_->isEmpty() )
	return nullptr;

    RefMan<Pick::Set> ps = uiCreatePicks::getPickSet();
    const int dpssize = dps_->size();

    for ( DataPointSet::RowID idx=0; idx<dpssize; idx++ )
    {
	const DataPointSet::Pos pos( dps_->pos(idx) );
	ps->add( Pick::Location(Coord3(pos.coord(),pos.z())) );
    }

    return ps;
}



// uiGenRandPicks2D
uiGenRandPicks2D::uiGenRandPicks2D( uiParent* p, const BufferStringSet& hornms,
				    const BufferStringSet& lnms )
    : uiCreatePicks(p,false,false)
    , createClicked(this)
    , hornms_(hornms)
    , geomfld_(nullptr)
    , linenms_(lnms)
{
    nrfld_ = new uiGenInput( this, tr("Number of points to generate"),
			     IntInpSpec(defnrpicks,1) );

    if ( hornms_.size() )
    {
	horselfld_ = new uiLabeledComboBox( this, mJoinUiStrs(sHorizon(),
							    sSelection()) );
	horselfld_->box()->addItem( uiStrings::sSelect() );
	horselfld_->box()->addItems( hornms_ );
	horselfld_->box()->selectionChanged.notify(
		mCB(this,uiGenRandPicks2D,hor1Sel) );
	horsel2fld_ = new uiComboBox( this, "" );
	horsel2fld_->addItem( uiStrings::sSelect()  );
	horsel2fld_->addItems( hornms_ );
	horsel2fld_->selectionChanged.notify(
		mCB(this,uiGenRandPicks2D,hor2Sel) );
    }

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Line(s)") );
    linenmfld_ = new uiListBox( this, su, "linenames" );
    linenmfld_->addItems( lnms );
    linenmfld_->attach( alignedBelow, nrfld_ );

    if ( hornms.size() )
    {
	geomfld_ = new uiGenInput( this, uiStrings::sGeometry(),
				     StringListInpSpec(sGeoms2D) );
	geomfld_->attach( alignedBelow, linenmfld_ );
	geomfld_->valueChanged.notify( mCB(this,uiGenRandPicks2D,geomSel) );
	horselfld_->attach( alignedBelow, geomfld_ );
	horsel2fld_->attach( rightOf, horselfld_ );
    }

    uiString zlbl = uiStrings::phrJoinStrings(uiStrings::sZRange(),
						       SI().getUiZUnitString());
    StepInterval<float> survzrg = SI().zRange(false);
    Interval<float> inpzrg( survzrg.start_, survzrg.stop_ );
    inpzrg.scale( sCast(float,SI().zDomain().userFactor()) );
    zfld_ = new uiGenInput( this, zlbl, FloatInpIntervalSpec(inpzrg) );
    if ( geomfld_ ) zfld_->attach( alignedBelow, geomfld_ );
    else zfld_->attach( alignedBelow, linenmfld_ );

    addStdFields( zfld_->attachObj() );
    preFinalize().notify( mCB(this,uiGenRandPicks2D,geomSel) );
}


uiGenRandPicks2D::~uiGenRandPicks2D()
{}


void uiGenRandPicks2D::hor1Sel( CallBacker* )
{
    horSel( horselfld_->box(), horsel2fld_ );
}


void uiGenRandPicks2D::hor2Sel( CallBacker* )
{
    horSel( horsel2fld_, horselfld_->box() );
}


void uiGenRandPicks2D::horSel( uiComboBox* sel, uiComboBox* tosel )
{
    const BufferString nm = sel->text();
    const BufferString curnm = tosel->text();
    const int idx = hornms_.indexOf( nm.buf() );
    BufferStringSet hornms( hornms_ );

    if ( idx >= 0 ) hornms.removeSingle( idx );

    tosel->setEmpty();
    tosel->addItem( uiStrings::sSelect()  );
    tosel->addItems( hornms );
    tosel->setCurrentItem( curnm.buf() );
}


void uiGenRandPicks2D::geomSel( CallBacker* )
{
    if ( !geomfld_ ) return;

    const int geomtyp = geomfld_->getIntValue();
    zfld_->display( geomtyp==0 );
    horselfld_->display( geomtyp!=0 );
    horsel2fld_->display( geomtyp==2 );
}


void uiGenRandPicks2D::mkRandPars()
{
    randpars_.nr_ = nrfld_->getIntValue();
    randpars_.needhor_ = geomfld_ && geomfld_->getIntValue();

    linenmfld_->getChosen( randpars_.linenms_ );
    if ( randpars_.needhor_ )
    {
	randpars_.horidx_ = hornms_.indexOf( horselfld_->box()->text() );
	randpars_.horidx2_ = -1;
	if ( geomfld_->getIntValue() == 2 )
	    randpars_.horidx2_ = hornms_.indexOf( horsel2fld_->text() );
    }
    else
    {
	randpars_.zrg_ = zfld_->getFInterval();
	randpars_.zrg_.scale( 1.f / SI().zDomain().userFactor() );
    }
}


bool uiGenRandPicks2D::acceptOK( CallBacker* )
{
    if ( !doAccept() )
	return false;

    const int choice = geomfld_ ? geomfld_->getIntValue() : 0;
    if ( choice )
    {
	const BufferString selstr = uiStrings::sSelect().getFullString();
	if ( selstr == horselfld_->box()->text() )
	    mErrRet(uiStrings::phrSelect(tr("a valid horizon")));
	if (choice == 2 && selstr == horsel2fld_->text() )
	    mErrRet(uiStrings::phrSelect(tr("a valid second horizon")));
    }
    else
    {
	Interval<float> zrg = zfld_->getFInterval();
	StepInterval<float> survzrg = SI().zRange(false);
	survzrg.scale( sCast(float,SI().zDomain().userFactor()) );
        if ( !survzrg.includes(zrg.start_,false) ||
             !survzrg.includes(zrg.stop_,false) )
	    mErrRet(uiStrings::phrEnter(tr("a valid Z Range")));
    }

    mkRandPars();
    defnrpicks = randpars_.nr_;

    return isModal();
}
