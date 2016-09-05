/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/


#include "uicreatepicks.h"

#include "uipossubsel.h"
#include "uiposfilterset.h"
#include "uimarkerstyle.h"
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
#include "trckeyzsampling.h"
#include "datainpspec.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "dbkey.h"
#include "picksetmanager.h"
#include "posprovider.h"
#include "randcolor.h"
#include "survinfo.h"
#include "statrand.h"
#include "datapointset.h"
#include "od_helpids.h"

static int defnrpicks_ = 500;
static int defsize_ = 3;
static const char* sGeoms2D[] = { "Z Range", "On Horizon",
				  "Between Horizons", 0 };
static const TypeSet<OD::MarkerStyle3D::Type>
		exclude_none( 1, OD::MarkerStyle3D::None );


uiNewPickSetDlg::uiNewPickSetDlg( uiParent* p, bool ispoly, const char* cat )
    : uiDialog(p,uiDialog::Setup(
			ispoly ? uiStrings::phrCreateNew(uiStrings::sPolygon())
			       : uiStrings::phrCreateNew(uiStrings::sPickSet()),
			       mNoDlgTitle,mODHelpKey(mFetchPicksHelpID)))
    , ispolygon_(ispoly)
    , category_(cat)
{
    nmfld_ = new uiGenInput( this,
		tr("Name for new %1").arg(ispolygon_ ? uiStrings::sPolygon() :
						       uiStrings::sPickSet()) );

    markerstylefld_ = new uiMarkerStyle3D( this, true,
		    Interval<int>(1,uiMarkerStyle3D::cDefMaxMarkerSize()),
		    ispolygon_ ? 0 : &exclude_none  );
    OD::MarkerStyle3D mstyle;
    mstyle.type_ = OD::MarkerStyle3D::Sphere;
    mstyle.size_ = defsize_;
    mstyle.color_ = getRandStdDrawColor();
    markerstylefld_->setMarkerStyle( mstyle );
    markerstylefld_->attach( alignedBelow, nmfld_ );
}


void uiNewPickSetDlg::attachStdFlds( bool mineabove, uiGroup* grp )
{
    if ( mineabove )
	nmfld_->attach( alignedBelow, grp );
    else
	grp->attach( alignedBelow, markerstylefld_ );
}


RefMan<Pick::Set> uiNewPickSetDlg::getEmptyPickSet() const
{
    const BufferString nm( nmfld_->text() );
    bool isreplace = Pick::SetMGR().nameExists( nm );
    RefMan<Pick::Set> ret;
    if ( !isreplace )
	ret = new Pick::Set( nm );
    else
    {
	const DBKey setid = Pick::SetMGR().getIDByName( nm );
	uiString msg = tr("A Pick Set with that name already exists.\n");
	if ( Pick::SetMGR().isLoaded( setid ) )
	{
	    msg.append( tr("You are currently using it."
			"\nPlease enter a different name."), true );
	    return ret;
	}
	msg.append( tr("Do you want to overwrite the existing data?"), true );
	if ( !uiMSG().askGoOn( msg ) )
	    return ret;

	ret = Pick::SetMGR().fetchForEdit( setid );
	if ( !ret )
	    ret = new Pick::Set( nm );
	else
	    ret->setEmpty();
    }

    OD::MarkerStyle3D mstyle;
    markerstylefld_->getMarkerStyle( mstyle );
    ret->setMarkerStyle( mstyle );
    ret->setConnection( ispolygon_ ? Pick::Set::Disp::Open
				   : Pick::Set::Disp::None );
    ret->setIsPolygon( ispolygon_ );
    ret->setCategory( category_ );

    return ret;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiNewPickSetDlg::acceptOK()
{
    RefMan<Pick::Set> newset = getEmptyPickSet();
    set_ = newset;
    if ( !set_ || !fillData(*set_) )
	return false;

    defsize_ = set_->dispSize();

    uiString errmsg = Pick::SetMGR().store( *set_ );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg )

    return true;
}


uiGenPosPicksDlg::uiGenPosPicksDlg( uiParent* p )
    : uiNewPickSetDlg(p,false)
    , posprovfld_(0)
{
    uiPosProvider::Setup psu( false, true, true );
    psu .seltxt( tr("Generate locations by") )
	.choicetype( uiPosProvider::Setup::All );
    posprovfld_ = new uiPosProvider( this, psu );
    posprovfld_->setExtractionDefaults();

    maxnrpickfld_ = new uiGenInput( this, tr("Maximum number of Picks"),
				    IntInpSpec(100) );
    maxnrpickfld_->attach( alignedBelow, posprovfld_ );

    uiPosFilterSet::Setup fsu( false );
    fsu.seltxt( uiStrings::phrRemove(tr("locations")) ).incprovs( true );
    posfiltfld_ = new uiPosFilterSetSel( this, fsu );
    posfiltfld_->attach( alignedBelow, maxnrpickfld_ );

    attachStdFlds( true, posfiltfld_ );
}


uiGenPosPicksDlg::~uiGenPosPicksDlg()
{
}


#define mSetCursor() MouseCursorManager::setOverride( MouseCursor::Wait )
#define mRestorCursor() MouseCursorManager::restoreOverride()

bool uiGenPosPicksDlg::fillData( Pick::Set& ps )
{
    PtrMan<Pos::Provider> prov = posprovfld_->createProvider();
    if ( !prov )
	mErrRet(toUiString("Internal: no Pos::Provider"))

    uiTaskRunner taskrunner( this );
    if ( !prov->initialize( &taskrunner ) )
	return false;

    mSetCursor();
    IOPar iop; posfiltfld_->fillPar( iop );
    PtrMan<Pos::Filter> filt = Pos::Filter::make( iop, prov->is2D() );
    if ( filt && !filt->initialize(&taskrunner) )
	{ mRestorCursor(); return false; }

    DataPointSet dps( prov->is2D() );
    if ( !dps.extractPositions(*prov,ObjectSet<DataColDef>(),filt,
				 &taskrunner) )
	return false;
    mRestorCursor();

    const int dpssize = dps.size();
    int size = maxnrpickfld_->getIntValue();
    if ( dpssize < size )
	size = dpssize;

    if ( size>50000 )
    {
	uiString msg = tr("PickSet would contain %1 "
			  "points which might consume unexpected time & memory."
			  "\n\nDo you want to continue?")
		     .arg(dpssize);
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    if ( dps.isEmpty() )
	mErrRet(tr("No matching locations found"))

    const bool usemaxnrpicks = dpssize > size;
    if ( !usemaxnrpicks )
	size = dpssize;

    Pos::SurvID survid = dps.bivSet().survID();
    for ( DataPointSet::RowID idx=0; idx<size; idx++ )
    {
	const int posidx = usemaxnrpicks ? Stats::randGen().getIndex( dpssize )
					 : idx;
	const DataPointSet::Pos pos( dps.pos(posidx) );
	Pick::Location pl( pos.coord(survid), pos.z() );
	if ( dps.is2D() )
	    pl.setSurvID( pos.binid_.inl() );
	ps.add( pl );
    }

    return true;
}



uiGenRandPicks2DDlg::uiGenRandPicks2DDlg( uiParent* p,
	const BufferStringSet& hornms, const BufferStringSet& lnms )
    : uiNewPickSetDlg(p,false)
    , geomfld_(0)
    , hornms_(hornms)
    , linenms_(lnms)
    , fillLocs(this)
{
    nrfld_ = new uiGenInput( this, tr("Number of picks to generate"),
		    IntInpSpec(defnrpicks_).setLimits(Interval<int>(1,10000)) );

    if ( hornms_.size() )
    {
	horselfld_ = new uiLabeledComboBox( this, mJoinUiStrs(sHorizon(),
							    sSelection()) );
	horselfld_->box()->addItem( uiStrings::sSelect() );
	horselfld_->box()->addItems( hornms_ );
	horselfld_->box()->selectionChanged.notify(mCB(this,
						uiGenRandPicks2DDlg,hor1Sel));
	horsel2fld_ = new uiComboBox( this, "" );
	horsel2fld_->addItem( uiStrings::sSelect()  );
	horsel2fld_->addItems( hornms_ );
	horsel2fld_->selectionChanged.notify( mCB(this,
						 uiGenRandPicks2DDlg,hor2Sel) );
    }

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Line(s)") );
    linenmfld_ = new uiListBox( this, su );
    linenmfld_->addItems( lnms );
    linenmfld_->attach( alignedBelow, nrfld_ );

    if ( hornms.size() )
    {
	geomfld_ = new uiGenInput( this, uiStrings::sGeometry(),
				     StringListInpSpec(sGeoms2D) );
	geomfld_->attach( alignedBelow, linenmfld_ );
	geomfld_->valuechanged.notify( mCB(this,uiGenRandPicks2DDlg,geomSel) );
	horselfld_->attach( alignedBelow, geomfld_ );
	horsel2fld_->attach( rightOf, horselfld_ );
    }

    uiString zlbl = uiStrings::phrJoinStrings(uiStrings::sZRange(),
						       SI().getUiZUnitString());
    StepInterval<float> survzrg = SI().zRange(false);
    Interval<float> inpzrg( survzrg.start, survzrg.stop );
    inpzrg.scale( mCast(float,SI().zDomain().userFactor()) );
    zfld_ = new uiGenInput( this, zlbl, FloatInpIntervalSpec(inpzrg) );
    if ( geomfld_ ) zfld_->attach( alignedBelow, geomfld_ );
    else zfld_->attach( alignedBelow, linenmfld_ );

    attachStdFlds( true, zfld_ );
    preFinalise().notify( mCB(this,uiGenRandPicks2DDlg,geomSel) );
}


void uiGenRandPicks2DDlg::hor1Sel( CallBacker* cb )
{
    horSel( horselfld_->box(), horsel2fld_ );
}


void uiGenRandPicks2DDlg::hor2Sel( CallBacker* cb )
{
    horSel( horsel2fld_, horselfld_->box() );
}


void uiGenRandPicks2DDlg::horSel( uiComboBox* sel, uiComboBox* tosel )
{
    const char* nm = sel->text();
    const char* curnm = tosel->text();
    const int idx = hornms_.indexOf( nm );
    BufferStringSet hornms( hornms_ );

    if ( idx >= 0 ) hornms.removeSingle( idx );

    tosel->setEmpty();
    tosel->addItem( uiStrings::sSelect()  );
    tosel->addItems( hornms );
    tosel->setCurrentItem( curnm );
}


void uiGenRandPicks2DDlg::geomSel( CallBacker* cb )
{
    if ( !geomfld_ ) return;

    const int geomtyp = geomfld_->getIntValue();
    zfld_->display( geomtyp==0 );
    horselfld_->display( geomtyp!=0 );
    horsel2fld_->display( geomtyp==2 );
}


void uiGenRandPicks2DDlg::mkRandPars()
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


bool uiGenRandPicks2DDlg::fillData( Pick::Set& ps )
{
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
	survzrg.scale( mCast(float,SI().zDomain().userFactor()) );
	if ( !survzrg.includes(zrg.start,false) ||
		!survzrg.includes(zrg.stop,false) )
	    mErrRet(uiStrings::phrEnter(tr("a valid Z Range")));
    }

    mkRandPars();
    defnrpicks_ = randpars_.nr_;

    fillLocs.trigger();
    return !ps.isEmpty();
}
