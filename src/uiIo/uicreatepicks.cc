/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id$";

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

static int defnrpicks = 500;
static const char* sGeoms3D[] = { "Volume", "On Horizon",
    				  "Between Horizons", 0};
static const char* sGeoms2D[] = { "Z Range", "On Horizon",
    				  "Between Horizons", 0 };


uiCreatePicks::uiCreatePicks( uiParent* p, bool aspoly )
    : uiDialog(p,uiDialog::Setup(
			aspoly ? "Polygon Creation" : "Pick Set Creation",
			aspoly ? "Create new Polygon" : "Create new PickSet",
			"105.0.0"))
    , aspolygon_(aspoly)
{
    nmfld_ = new uiGenInput( this,
		BufferString("Name for new ",aspoly ? "Polygon" : "PickSet") );
    colsel_ = new uiColorInput( this,
	    		      uiColorInput::Setup(getRandStdDrawColor()).
	   		      lbltxt("Color") );
    colsel_->attach( alignedBelow, nmfld_ );
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
    BufferString res = nmfld_->text();
    char* ptr = res.buf(); mTrimBlanks(ptr);
    if ( !*ptr )
	mErrRet( "Please enter a name" )

    name_ = ptr;
    return true;
}


uiGenPosPicks::uiGenPosPicks( uiParent* p )
    : uiCreatePicks(p)
    , posprovfld_(0)
    , dps_(0)
{
    setTitleText( "Create new pickset" );

    uiPosProvider::Setup psu( false, true, true );
    psu .seltxt( "Generate locations by" )
	.choicetype( uiPosProvider::Setup::All );
    posprovfld_ = new uiPosProvider( this, psu );
    posprovfld_->setExtractionDefaults();
    posprovfld_->attach( alignedBelow, colsel_);

    uiPosFilterSet::Setup fsu( false );
    fsu.seltxt( "Remove locations" ).incprovs( true );
    posfiltfld_ = new uiPosFilterSetSel( this, fsu );
    posfiltfld_->attach( alignedBelow, posprovfld_ );
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
    if ( dps_->isEmpty() )
	{ delete dps_; dps_ = 0; mErrRet("No matching locations found") }

    return true;   
}


Pick::Set* uiGenPosPicks::getPickSet() const
{
    if ( dps_->isEmpty() ) return 0;

    Pick::Set* ps = uiCreatePicks::getPickSet();
    for ( DataPointSet::RowID idx=0; idx<dps_->size(); idx++ )
    {
	const DataPointSet::Pos pos( dps_->pos(idx) );
	*ps += Pick::Location( pos.coord(), pos.z() );
    }

    return ps;
}




uiGenRandPicks2D::uiGenRandPicks2D( uiParent* p, const BufferStringSet& hornms,
       				  const BufferStringSet& lsets,
				  const TypeSet<BufferStringSet>& lnms )
    : uiCreatePicks(p)
    , geomfld_(0)
    , hornms_(hornms)
    , linenms_(lnms)
{
    setTitleText( "Create new pickset with random positions" );
    nrfld_ = new uiGenInput( this, "Number of picks to generate",
					 IntInpSpec(defnrpicks) );
    nrfld_->attach( alignedBelow, colsel_);

    if ( hornms_.size() )
    {
	horselfld_ = new uiLabeledComboBox( this, "Horizon selection" );
	horselfld_->box()->addItem( "Select" );
	horselfld_->box()->addItems( hornms_ );
	horselfld_->box()->selectionChanged.notify(mCB(this,
		    				    uiGenRandPicks2D,hor1Sel));
	horsel2fld_ = new uiComboBox( this, "" );
	horsel2fld_->addItem( "Select" );
	horsel2fld_->addItems( hornms_ );
	horsel2fld_->selectionChanged.notify( mCB(this,
		    				 uiGenRandPicks2D,hor2Sel) );
    }

    linesetfld_ = new uiGenInput( this, "Line Set",
	    			  StringListInpSpec(lsets) );
    linesetfld_->attach( alignedBelow, nrfld_ );
    linesetfld_->valuechanged.notify( mCB(this,uiGenRandPicks2D,lineSetSel) );

    linenmfld_ = new uiLabeledListBox( this, lnms[0], "Select Lines", true);
    linenmfld_->attach( alignedBelow, linesetfld_ );

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
    inpzrg.scale( SI().zFactor() );
    zfld_ = new uiGenInput( this, zlbl, FloatInpIntervalSpec(inpzrg) );
    if ( geomfld_ ) zfld_->attach( alignedBelow, geomfld_ );
    else zfld_->attach( alignedBelow, linenmfld_ );

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
    BufferString* bs = 0;
    if ( idx >= 0 ) bs = hornms.remove( idx );

    tosel->setEmpty();
    tosel->addItem( "Select" );
    tosel->addItems( hornms );
    tosel->setCurrentItem( curnm );
    if ( bs ) delete bs;
}



void uiGenRandPicks2D::geomSel( CallBacker* cb )
{
    if ( !geomfld_ ) return;

    const int geomtyp = geomfld_->getIntValue();
    zfld_->display( geomtyp==0 );
    horselfld_->display( geomtyp!=0 );
    horsel2fld_->display( geomtyp==2 );
}


void uiGenRandPicks2D::lineSetSel( CallBacker* cb )
{
    const int setidx = linesetfld_->getIntValue();
    linenmfld_->box()->setEmpty();
    if ( setidx<0 || setidx>=linenms_.size() ) return;

    linenmfld_->box()->addItems( linenms_[setidx] );
}


void uiGenRandPicks2D::mkRandPars()
{
    randpars_.nr_ = nrfld_->getIntValue();
    randpars_.needhor_ = geomfld_ && geomfld_->getIntValue();

    randpars_.lsetidx_ = linesetfld_->getIntValue();
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
	randpars_.zrg_.scale( 1. / SI().zFactor() );
    }
}


bool uiGenRandPicks2D::acceptOK( CallBacker* c )
{
    if ( !uiCreatePicks::acceptOK(c) )
	return false;

    const int choice = geomfld_ ? geomfld_->getIntValue() : 0;
    if ( choice )
    {
	if ( !strcmp(horselfld_->box()->text(),"Select") )
	    mErrRet( "Please Select a valid horizon" );
	if ( choice==2 && !strcmp(horsel2fld_->text(),"Select") )
	    mErrRet( "Please Select a valid second horizon" );
    }
    else
    {
	Interval<float> zrg = zfld_->getFInterval();
	StepInterval<float> survzrg = SI().zRange(false);
	survzrg.scale( SI().zFactor() );
	if ( !survzrg.includes(zrg.start,false) || !survzrg.includes(zrg.stop,false) )
		mErrRet( "Please Enter a valid Z Range" );
    }

    mkRandPars();
    defnrpicks = randpars_.nr_;
    return true;
}
