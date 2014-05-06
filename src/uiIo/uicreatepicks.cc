/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uicreatepicks.h"

#include "uipossubsel.h"
#include "uiposfilterset.h"
#include "uiposprovider.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uimsg.h"

#include "color.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "pickset.h"
#include "picksettr.h"
#include "posprovider.h"
#include "randcolor.h"
#include "survinfo.h"
#include "statrand.h"
#include "datapointset.h"
#include "od_helpids.h"

FixedString selstr( "Select" );

static int defnrpicks = 500;
static const char* sGeoms2D[] = { "Z Range", "On Horizon",
				  "Between Horizons", 0 };


uiCreatePicks::uiCreatePicks( uiParent* p, bool aspoly, bool addstdflds )
    : uiDialog(p,uiDialog::Setup(
			aspoly ? "Create new Polygon" : "Create new PickSet",
			mNoDlgTitle,mODHelpKey(mFetchPicksHelpID)))
    , aspolygon_(aspoly)
{
    if ( addstdflds )
	addStdFields( 0 );
}


void uiCreatePicks::addStdFields( uiObject* lastobject )
{
    nmfld_ = new uiGenInput( this,
		BufferString("Name for new ",aspolygon_ ? "Polygon"
							: "PickSet") );
    colsel_ = new uiColorInput( this,
			      uiColorInput::Setup(getRandStdDrawColor()).
			      lbltxt("Color") );
    colsel_->attach( alignedBelow, nmfld_ );
    if ( lastobject )
	nmfld_->attach( alignedBelow, lastobject );
}


Pick::Set* uiCreatePicks::getPickSet() const
{
    Pick::Set* ret = new Pick::Set( name_ );
    ret->disp_.color_ = colsel_->color();
    ret->disp_.connect_ = aspolygon_ ? Pick::Set::Disp::Open
				  : Pick::Set::Disp::None;
    return ret;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiCreatePicks::acceptOK( CallBacker* )
{
    name_.set( nmfld_->text() ).trimBlanks();
    return true;
}


uiGenPosPicks::uiGenPosPicks( uiParent* p )
    : uiCreatePicks(p,false,false)
    , posprovfld_(0)
    , dps_(0)
{
    uiPosProvider::Setup psu( false, true, true );
    psu .seltxt( "Generate locations by" )
	.choicetype( uiPosProvider::Setup::All );
    posprovfld_ = new uiPosProvider( this, psu );
    posprovfld_->setExtractionDefaults();

    maxnrpickfld_ = new uiGenInput( this, "Maximum number of Picks", 
				    IntInpSpec(100) );
    maxnrpickfld_->attach( alignedBelow, posprovfld_ );

    uiPosFilterSet::Setup fsu( false );
    fsu.seltxt( "Remove locations" ).incprovs( true );
    posfiltfld_ = new uiPosFilterSetSel( this, fsu );
    posfiltfld_->attach( alignedBelow, maxnrpickfld_ );

    addStdFields( posfiltfld_->attachObj() );
}


uiGenPosPicks::~uiGenPosPicks()
{
    delete dps_;
}


#define mSetCursor() MouseCursorManager::setOverride( MouseCursor::Wait )
#define mRestorCursor() MouseCursorManager::restoreOverride()

bool uiGenPosPicks::acceptOK( CallBacker* c )
{
    if ( !posprovfld_ ) return true;

    if ( !uiCreatePicks::acceptOK(c) )
	return false;

    PtrMan<Pos::Provider> prov = posprovfld_->createProvider();
    if ( !prov )
	mErrRet("Internal: no Pos::Provider")

    uiTaskRunner tr( this );
    if ( !prov->initialize( &tr ) )
	return false;

    mSetCursor();
    IOPar iop; posfiltfld_->fillPar( iop );
    PtrMan<Pos::Filter> filt = Pos::Filter::make( iop, prov->is2D() );
    if ( filt && !filt->initialize(&tr) )
	{ mRestorCursor(); return false; }

    dps_ = new DataPointSet( *prov, ObjectSet<DataColDef>(), filt );
    mRestorCursor();

    const int dpssize = dps_->size();
    int size = maxnrpickfld_->getIntValue();
    if ( dpssize < size )
	size = dpssize;

    if ( size>50000 )
    {
	BufferString msg( "PickSet would contain " );
	msg += dpssize;
	msg += " points which might consume unexpected time & memory.";
	msg += "Do you want to continue?";
	if ( !uiMSG().askGoOn(msg) )
	{
	    mRestorCursor();
	    delete dps_; dps_ = 0;
	    return false;
	}
    }

    if ( dps_->isEmpty() )
	{ delete dps_; dps_ = 0; mErrRet("No matching locations found") }

    return true;
}


Pick::Set* uiGenPosPicks::getPickSet() const
{
    if ( dps_->isEmpty() ) return 0;

    Stats::randGen().init();
    Pick::Set* ps = uiCreatePicks::getPickSet();
    const int dpssize = dps_->size();
    int size = maxnrpickfld_->getIntValue();
    const bool usemaxnrpicks = dpssize > size;
    if ( !usemaxnrpicks )
	size = dpssize;

    for ( DataPointSet::RowID idx=0; idx<size; idx++ )
    {
	const int posidx = usemaxnrpicks ? Stats::randGen().getIndex( dpssize ) 
					 : idx;
	const DataPointSet::Pos pos( dps_->pos(posidx) );
	*ps += Pick::Location( pos.coord(), pos.z() );
    }

    return ps;
}




uiGenRandPicks2D::uiGenRandPicks2D( uiParent* p, const BufferStringSet& hornms,
				    const BufferStringSet& lnms )
    : uiCreatePicks(p,false,false)
    , geomfld_(0)
    , hornms_(hornms)
    , linenms_(lnms)
{
    nrfld_ = new uiGenInput( this, "Number of picks to generate",
		    IntInpSpec(defnrpicks).setLimits(Interval<int>(1,10000)) );

    if ( hornms_.size() )
    {
	horselfld_ = new uiLabeledComboBox( this, "Horizon selection" );
	horselfld_->box()->addItem( selstr );
	horselfld_->box()->addItems( hornms_ );
	horselfld_->box()->selectionChanged.notify(mCB(this,
						    uiGenRandPicks2D,hor1Sel));
	horsel2fld_ = new uiComboBox( this, "" );
	horsel2fld_->addItem( selstr );
	horsel2fld_->addItems( hornms_ );
	horsel2fld_->selectionChanged.notify( mCB(this,
						 uiGenRandPicks2D,hor2Sel) );
    }

    linenmfld_ = new uiLabeledListBox( this, lnms, "Select Lines", true);
    linenmfld_->attach( alignedBelow, nrfld_ );

    if ( hornms.size() )
    {
	geomfld_ = new uiGenInput( this, "Geometry",
				     StringListInpSpec(sGeoms2D) );
	geomfld_->attach( alignedBelow, linenmfld_ );
	geomfld_->valuechanged.notify( mCB(this,uiGenRandPicks2D,geomSel) );
	horselfld_->attach( alignedBelow, geomfld_ );
	horsel2fld_->attach( rightOf, horselfld_ );
    }

    BufferString zlbl = "Z Range";
    zlbl += SI().getZUnitString();
    StepInterval<float> survzrg = SI().zRange(false);
    Interval<float> inpzrg( survzrg.start, survzrg.stop );
    inpzrg.scale( mCast(float,SI().zDomain().userFactor()) );
    zfld_ = new uiGenInput( this, zlbl, FloatInpIntervalSpec(inpzrg) );
    if ( geomfld_ ) zfld_->attach( alignedBelow, geomfld_ );
    else zfld_->attach( alignedBelow, linenmfld_ );

    addStdFields( zfld_->attachObj() );
    preFinalise().notify( mCB(this,uiGenRandPicks2D,geomSel) );
}


void uiGenRandPicks2D::hor1Sel( CallBacker* cb )
{
    horSel( horselfld_->box(), horsel2fld_ );
}


void uiGenRandPicks2D::hor2Sel( CallBacker* cb )
{
    horSel( horsel2fld_, horselfld_->box() );
}


void uiGenRandPicks2D::horSel( uiComboBox* sel, uiComboBox* tosel )
{
    const char* nm = sel->text();
    const char* curnm = tosel->text();
    const int idx = hornms_.indexOf( nm );
    BufferStringSet hornms( hornms_ );

    if ( idx >= 0 ) hornms.removeSingle( idx );

    tosel->setEmpty();
    tosel->addItem( selstr );
    tosel->addItems( hornms );
    tosel->setCurrentItem( curnm );
}



void uiGenRandPicks2D::geomSel( CallBacker* cb )
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

    linenmfld_->box()->getSelectedItems( randpars_.linenms_ );
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


bool uiGenRandPicks2D::acceptOK( CallBacker* c )
{
    if ( !uiCreatePicks::acceptOK(c) )
	return false;

    const int choice = geomfld_ ? geomfld_->getIntValue() : 0;
    if ( choice )
    {
	if ( selstr==horselfld_->box()->text() )
	    mErrRet( "Please Select a valid horizon" );
	if ( choice==2 && selstr==horsel2fld_->text() )
	    mErrRet( "Please Select a valid second horizon" );
    }
    else
    {
	Interval<float> zrg = zfld_->getFInterval();
	StepInterval<float> survzrg = SI().zRange(false);
	survzrg.scale( mCast(float,SI().zDomain().userFactor()) );
	if ( !survzrg.includes(zrg.start,false) ||
		!survzrg.includes(zrg.stop,false) )
		mErrRet( "Please Enter a valid Z Range" );
    }

    mkRandPars();
    defnrpicks = randpars_.nr_;
    return true;
}
