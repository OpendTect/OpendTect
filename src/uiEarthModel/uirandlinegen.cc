/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          January 2007
________________________________________________________________________

-*/

#include "uirandlinegen.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emrandlinegen.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survinfo.h"
#include "pickset.h"
#include "picksettr.h"
#include "polygon.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiselsurvranges.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uiselsimple.h"
#include "uispinbox.h"
#include "uilabel.h"
#include "uistrings.h"
#include "od_helpids.h"

#include <limits.h>

uiString uiGenRanLinesByContour::sSpecGenPar()
{ return tr("Specify generation parameters"); }

uiString uiGenRanLinesByContour::sDlgTitle()
{ return uiStrings::phrCreate( uiStrings::sRandomLine() ); }

uiGenRanLinesByContour::uiGenRanLinesByContour( uiParent* p )
    : uiDialog( p, Setup(uiGenRanLinesByContour::sDlgTitle(),
			 uiGenRanLinesByContour::sSpecGenPar(),
			 mODHelpKey(mGenRanLinesByContourHelpID) ) )
    , horctio_(*mMkCtxtIOObj(EMHorizon3D))
    , polyctio_(*mMkCtxtIOObj(PickSet))
    , rlsctio_(*mMkCtxtIOObj(RandomLineSet))
{
    IOM().to( horctio_.ctxt_.getSelKey() );
    rlsctio_.ctxt_.forread_ = false;
    polyctio_.ctxt_.toselect_.require_.set( sKey::Type(), sKey::Polygon() );

    infld_ = new uiIOObjSel( this, horctio_, 
			     uiStrings::phrInput(uiStrings::sHorizon(1)));
    uiIOObjSel::Setup osu( tr("Within polygon") ); osu.optional( true );
    polyfld_ = new uiIOObjSel( this, polyctio_, osu );
    polyfld_->attach( alignedBelow, infld_ );

    StepInterval<float> sizrg( SI().zRange(true) );
    sizrg.scale( mCast(float,SI().zDomain().userFactor()) );
    StepInterval<float> suggestedzrg( sizrg );
    suggestedzrg.step *= 10;
    contzrgfld_ = new uiGenInput( this, tr("Contour Z range"),
			FloatInpIntervalSpec(suggestedzrg) );
    contzrgfld_->attach( alignedBelow, polyfld_ );

    const CallBack locb( mCB(this,uiGenRanLinesByContour,largestOnlyChg) );
    largestfld_ = new uiCheckBox( this, tr("Only use largest"), locb );
    largestfld_->setChecked( false );
    nrlargestfld_ = new uiSpinBox( this, 0, "Number of largest" );
    nrlargestfld_->setInterval( 1, INT_MAX );
    nrlargestfld_->attach( rightOf, largestfld_ );
    largestendfld_ = new uiLabel( this, tr("Z-contour(s)") );
    nrlargestfld_->attach( alignedBelow, contzrgfld_ );
    largestfld_->attach( leftOf, nrlargestfld_ );
    largestendfld_->attach( rightOf, nrlargestfld_ );
    largestOnlyChg( 0 );

    vtxthreshfld_ = new uiLabeledSpinBox( this, tr("Minimum number of points"));
    vtxthreshfld_->box()->setInterval( 2, INT_MAX );
    vtxthreshfld_->attach( alignedBelow, nrlargestfld_ );

    uiString fldnm = tr("Random line Z range");
    const float wdth = 50 * sizrg.step;
    relzrgfld_ = new uiGenInput( this, fldnm,
			FloatInpIntervalSpec(Interval<float>(-wdth,wdth)) );
    relzrgfld_->attach( alignedBelow, vtxthreshfld_ );
    Interval<float> abszrg; assign( abszrg, sizrg );
    abszrgfld_ = new uiGenInput( this, fldnm, FloatInpIntervalSpec(abszrg) );
    abszrgfld_->attach( alignedBelow, vtxthreshfld_ );
    const CallBack cb( mCB(this,uiGenRanLinesByContour,isrelChg) );
    isrelfld_ = new uiCheckBox( this, tr("Relative to horizon"), cb );
    isrelfld_->setChecked( true );
    isrelfld_->attach( rightOf, abszrgfld_ );

    outfld_ = new uiIOObjSel( this, rlsctio_, tr("Random Line set") );
    outfld_->attach( alignedBelow, relzrgfld_ );

    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );

    postFinalise().notify( cb );
}


uiGenRanLinesByContour::~uiGenRanLinesByContour()
{
    delete horctio_.ioobj_; delete &horctio_;
    delete polyctio_.ioobj_; delete &polyctio_;
    delete rlsctio_.ioobj_; delete &rlsctio_;
}


bool uiGenRanLinesByContour::dispOnCreation()
{
    return dispfld_->isChecked();
}


const char* uiGenRanLinesByContour::getNewSetID() const
{
    BufferString* multid = new BufferString( rlsctio_.ioobj_->key().buf() );
    return rlsctio_.ioobj_ ? multid->buf() : 0;
}


void uiGenRanLinesByContour::largestOnlyChg( CallBacker* )
{ nrlargestfld_->setSensitive( largestfld_->isChecked() ); }


void uiGenRanLinesByContour::isrelChg( CallBacker* )
{
    const bool isrel = isrelfld_->isChecked();
    relzrgfld_->display( isrel );
    abszrgfld_->display( !isrel );
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiGenRanLinesByContour::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet(uiStrings::phrSelect(uiStrings::phrInput(
						    uiStrings::sHorizon(1))))
    if ( !outfld_->commitInput() )
	mErrRet((outfld_->isEmpty() ?
	    uiStrings::phrSelect(uiStrings::phrOutput(
	    uiStrings::phrJoinStrings(uiStrings::sRandomLine(),
	    uiStrings::sSet()))) : uiStrings::sEmptyString()))

    PtrMan< ODPolygon<float> > poly = 0;
    if ( polyfld_->isChecked() )
    {
	polyfld_->commitInput();
	BufferString msg;
	if ( polyctio_.ioobj_ )
	    poly = PickSetTranslator::getPolygon( *polyctio_.ioobj_, msg );
	else
	    msg = "Please select the polygon, or uncheck";
	if ( !poly )
	   mErrRet(mToUiStringTodo(msg))
    }

    uiTaskRunner taskrunner( this ); EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.loadIfNotFullyLoaded( horctio_.ioobj_->key(),
	    					   &taskrunner );
    mDynamicCastGet( EM::Horizon3D*, hor, emobj )
    if ( !hor ) return false;
    hor->ref();

    StepInterval<float> contzrg = contzrgfld_->getFStepInterval();
    const bool isrel = isrelfld_->isChecked();
    Interval<float> linezrg = (isrel?relzrgfld_:abszrgfld_)->getFStepInterval();
    const float zfac = 1.f / SI().zDomain().userFactor();
    contzrg.scale( zfac ); linezrg.scale( zfac );

    EM::RandomLineSetByContourGenerator::Setup cgsu( isrel );
    cgsu.contzrg( contzrg ).linezrg( linezrg );
    if ( poly ) cgsu.selpoly_ = poly;
    cgsu.minnrvertices_ = vtxthreshfld_->box()->getIntValue();
    if ( largestfld_->isChecked() )
	cgsu.nrlargestonly_ = nrlargestfld_->getIntValue();
    EM::RandomLineSetByContourGenerator gen( *hor, cgsu );
    Geometry::RandomLineSet rls;
    gen.createLines( rls );
    hor->unRef();

    const int rlssz = rls.size();
    if ( rlssz < 1 )
       mErrRet(uiStrings::phrCannotFind(tr("a contour in range")))
    else
    {
	BufferString emsg;
	if ( !RandomLineSetTranslator::store(rls,rlsctio_.ioobj_,emsg) )
	   mErrRet(mToUiStringTodo(emsg))
    }

    uiMSG().message(tr("Created %1%2").arg(rls.size())
		  .arg(rls.size()>1 ? tr(" lines") : tr(" line")));
    return true;
}


uiGenRanLinesByShift::uiGenRanLinesByShift( uiParent* p )
    : uiDialog( p, Setup(uiGenRanLinesByContour::sDlgTitle(),
			 uiGenRanLinesByContour::sSpecGenPar(),
			 mODHelpKey(mGenRanLinesByShiftHelpID) ) )
    , inctio_(*mMkCtxtIOObj(RandomLineSet))
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt_.forread_ = false;

    infld_ = new uiIOObjSel( this, inctio_, 
			     uiStrings::phrInput(uiStrings::sRandomLine()));

    const BinID bid1( 1, 1 );
    const BinID bid2( 1 + SI().sampling(false).hsamp_.step_.inl(), 1 );
    const Coord c1( SI().transform(bid1) );
    const Coord c2( SI().transform(bid2) );
    distfld_ = new uiGenInput( this, tr("Distance from input"),
				    FloatInpSpec((float)( 4*c1.distTo(c2)) ));
    distfld_->attach( alignedBelow, infld_ );

    const char* strs[] = { "Left", "Right", "Both", 0 };
    sidefld_ = new uiGenInput(this, tr("Direction"), 
			     StringListInpSpec(strs));
    sidefld_->setValue( 2 );
    sidefld_->attach( alignedBelow, distfld_ );

    outfld_ = new uiIOObjSel( this, outctio_, 
			      uiStrings::phrOutput(uiStrings::sRandomLine()));
    outfld_->attach( alignedBelow, sidefld_ );

    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );
}


uiGenRanLinesByShift::~uiGenRanLinesByShift()
{
    delete inctio_.ioobj_; delete &inctio_;
    delete outctio_.ioobj_; delete &outctio_;
}


bool uiGenRanLinesByShift::dispOnCreation()
{
    return dispfld_->isChecked();
}


const char* uiGenRanLinesByShift::getNewSetID() const
{
    BufferString* multid = new BufferString( outctio_.ioobj_->key().buf() );
    return outctio_.ioobj_ ? multid->buf() : 0;
}


bool uiGenRanLinesByShift::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet(uiStrings::phrSelect(uiStrings::phrInput(
	uiStrings::phrJoinStrings(uiStrings::sRandomLine(),tr("(Set)")))))
    if ( !outfld_->commitInput() )
	mErrRet((outfld_->isEmpty() ?
	       uiStrings::phrSelect(uiStrings::phrInput(
	       uiStrings::phrJoinStrings(uiStrings::sRandomLine(),tr("(Set)"))))
	       : uiStrings::sEmptyString()) )

    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,inctio_.ioobj_,msg) )
	mErrRet(mToUiStringTodo(msg))

    int lnr = 0;
    if ( inprls.size() > 1 )
    {
	BufferStringSet lnms;
	for ( int idx=0; idx<inprls.size(); idx++ )
	    lnms.add( inprls.lines()[idx]->name() );
	uiSelectFromList dlg( this,
		    uiSelectFromList::Setup(uiStrings::phrSelect(tr("one line"))
			      ,lnms) );
	if ( !dlg.go() ) return false;
	lnr = dlg.selection();
    }

    const int choice = sidefld_->getIntValue();
    EM::RandomLineByShiftGenerator gen( inprls, distfld_->getFValue(),
				    choice == 0 ? -1 : (choice == 1 ? 1 : 0) );
    Geometry::RandomLineSet outrls; gen.generate( outrls, lnr );
    if ( outrls.isEmpty() )
	mErrRet(tr("Not enough input points to create output"))

    if ( !RandomLineSetTranslator::store(outrls,outctio_.ioobj_,msg) )
	mErrRet(mToUiStringTodo(msg))

    return true;
}


uiGenRanLineFromPolygon::uiGenRanLineFromPolygon( uiParent* p )
    : uiDialog( p, Setup(uiGenRanLinesByContour::sDlgTitle(),
			 uiGenRanLinesByContour::sSpecGenPar(),
			 mODHelpKey(mGenRanLinesFromPolygonHelpID) ) )
    , inctio_(*mMkCtxtIOObj(PickSet))
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt_.forread_ = false;
    inctio_.ctxt_.toselect_.require_.set( sKey::Type(), sKey::Polygon() );

    infld_ = new uiIOObjSel( this, inctio_, 
			     uiStrings::phrInput(uiStrings::sPolygon()) );
    
    zrgfld_ = new uiSelZRange( this, true );
    zrgfld_->attach( alignedBelow, infld_ );
    outfld_ = new uiIOObjSel( this, outctio_,
			      uiStrings::phrOutput(uiStrings::sRandomLine()));
    outfld_->attach( alignedBelow, zrgfld_ );
    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );
}


uiGenRanLineFromPolygon::~uiGenRanLineFromPolygon()
{
    delete inctio_.ioobj_; delete &inctio_;
    delete outctio_.ioobj_; delete &outctio_;
}


const char* uiGenRanLineFromPolygon::getNewSetID() const
{
    BufferString* multid = new BufferString( outctio_.ioobj_->key().buf() );
    return outctio_.ioobj_ ? multid->buf() : 0;
}


bool uiGenRanLineFromPolygon::dispOnCreation()
{
    return dispfld_->isChecked();
}


bool uiGenRanLineFromPolygon::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet(uiStrings::phrSelect(uiStrings::phrInput(
							uiStrings::sPolygon())))
    if ( !outfld_->commitInput() )
	mErrRet((outfld_->isEmpty() ?
		uiStrings::phrSelect(uiStrings::phrOutput(
						      uiStrings::sRandomLine()))
		: uiStrings::sEmptyString()))

    PtrMan< ODPolygon<float> > poly = 0;
    BufferString msg;
    poly = PickSetTranslator::getPolygon( *inctio_.ioobj_, msg );
    if ( !poly )
       mErrRet(mToUiStringTodo(msg))

    RefMan<Geometry::RandomLine> rl = new Geometry::RandomLine;
    for ( int idx=0; idx<poly->size(); idx++ )
    {
	Geom::Point2D<float> pt = poly->getVertex( idx );
	BinID bid( mNINT32(pt.x), mNINT32(pt.y) );
	rl->addNode( bid );
    }
    if ( rl->isEmpty() )
	{ mErrRet(tr("Empty input polygon")) }
    rl->setZRange( zrgfld_->getRange() );

    Geometry::RandomLineSet outrls;
    outrls.addLine( *rl );
    if ( !RandomLineSetTranslator::store(outrls,outctio_.ioobj_,msg) )
	mErrRet(mToUiStringTodo(msg))

    return true;
}
