/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          January 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uirandlinegen.cc,v 1.29 2012-08-10 04:11:27 cvssalil Exp $";

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

#include <limits.h>

uiGenRanLinesByContour::uiGenRanLinesByContour( uiParent* p )
    : uiDialog( p, Setup("Create Random Lines","Specify generation parameters",
			 "109.0.1") )
    , horctio_(*mMkCtxtIOObj(EMHorizon3D))
    , polyctio_(*mMkCtxtIOObj(PickSet))
    , rlsctio_(*mMkCtxtIOObj(RandomLineSet))
{
    IOM().to( horctio_.ctxt.getSelKey() );
    rlsctio_.ctxt.forread = false;
    polyctio_.ctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );

    infld_ = new uiIOObjSel( this, horctio_, "Input Horizon" );
    uiIOObjSel::Setup osu( "Within polygon" ); osu.optional( true );
    polyfld_ = new uiIOObjSel( this, polyctio_, osu );
    polyfld_->attach( alignedBelow, infld_ );

    StepInterval<float> sizrg( SI().zRange(true) );
    sizrg.scale( SI().zDomain().userFactor() );
    StepInterval<float> suggestedzrg( sizrg );
    suggestedzrg.step *= 10;
    contzrgfld_ = new uiGenInput( this, "Contour Z range",
			FloatInpIntervalSpec(suggestedzrg) );
    contzrgfld_->attach( alignedBelow, polyfld_ );

    const CallBack locb( mCB(this,uiGenRanLinesByContour,largestOnlyChg) );
    largestfld_ = new uiCheckBox( this, "Only use largest", locb );
    largestfld_->setChecked( false );
    nrlargestfld_ = new uiSpinBox( this, 0, "Number of largest" );
    nrlargestfld_->setInterval( 1, INT_MAX );
    nrlargestfld_->attach( rightOf, largestfld_ );
    largestendfld_ = new uiLabel( this, "Z-contour(s)" );
    nrlargestfld_->attach( alignedBelow, contzrgfld_ );
    largestfld_->attach( leftOf, nrlargestfld_ );
    largestendfld_->attach( rightOf, nrlargestfld_ );
    largestOnlyChg( 0 );

    vtxthreshfld_ = new uiLabeledSpinBox( this, "Minimum number of points" );
    vtxthreshfld_->box()->setInterval( 2, INT_MAX );
    vtxthreshfld_->attach( alignedBelow, nrlargestfld_ );

    static const char* fldnm = "Random line Z range";
    const float wdth = 50 * sizrg.step;
    relzrgfld_ = new uiGenInput( this, fldnm,
			FloatInpIntervalSpec(Interval<float>(-wdth,wdth)) );
    relzrgfld_->attach( alignedBelow, vtxthreshfld_ );
    Interval<float> abszrg; assign( abszrg, sizrg );
    abszrgfld_ = new uiGenInput( this, fldnm, FloatInpIntervalSpec(abszrg) );
    abszrgfld_->attach( alignedBelow, vtxthreshfld_ );
    const CallBack cb( mCB(this,uiGenRanLinesByContour,isrelChg) );
    isrelfld_ = new uiCheckBox( this, "Relative to horizon", cb );
    isrelfld_->setChecked( true );
    isrelfld_->attach( rightOf, abszrgfld_ );

    outfld_ = new uiIOObjSel( this, rlsctio_, "Random Line set" );
    outfld_->attach( alignedBelow, relzrgfld_ );
    
    dispfld_ = new uiCheckBox( this, "Display Random Line on creation" );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );

    postFinalise().notify( cb );
}


uiGenRanLinesByContour::~uiGenRanLinesByContour()
{
    delete horctio_.ioobj; delete &horctio_;
    delete polyctio_.ioobj; delete &polyctio_;
    delete rlsctio_.ioobj; delete &rlsctio_;
}


bool uiGenRanLinesByContour::dispOnCreation()
{
    return dispfld_->isChecked();
}


const char* uiGenRanLinesByContour::getNewSetID() const
{
    BufferString* multid = new BufferString( rlsctio_.ioobj->key().buf() );
    return rlsctio_.ioobj ? multid->buf() : 0;
}


void uiGenRanLinesByContour::largestOnlyChg( CallBacker* )
{ nrlargestfld_->setSensitive( largestfld_->isChecked() ); }


void uiGenRanLinesByContour::isrelChg( CallBacker* )
{
    const bool isrel = isrelfld_->isChecked();
    relzrgfld_->display( isrel );
    abszrgfld_->display( !isrel );
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiGenRanLinesByContour::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet("Please select the input horizon")
    if ( !outfld_->commitInput() )
	mErrRet(outfld_->isEmpty() ?
		"Please select the output random line set" : 0)

    PtrMan< ODPolygon<float> > poly = 0;
    if ( polyfld_->isChecked() )
    {
	polyfld_->commitInput();
	BufferString msg;
	if ( polyctio_.ioobj )
	    poly = PickSetTranslator::getPolygon( *polyctio_.ioobj, msg );
	else
	    msg = "Please select the polygon, or uncheck";
	if ( !poly )
	   mErrRet(msg)
    }
    
    uiTaskRunner taskrunner( this ); EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.loadIfNotFullyLoaded( horctio_.ioobj->key(),
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
    cgsu.minnrvertices_ = vtxthreshfld_->box()->getValue();
    if ( largestfld_->isChecked() )
	cgsu.nrlargestonly_ = nrlargestfld_->getValue();
    EM::RandomLineSetByContourGenerator gen( *hor, cgsu );
    Geometry::RandomLineSet rls;
    gen.createLines( rls );
    hor->unRef();

    const int rlssz = rls.size();
    if ( rlssz < 1 )
       mErrRet("Could not find a contour in range")
    else
    {
	BufferString emsg;
	if ( !RandomLineSetTranslator::store(rls,rlsctio_.ioobj,emsg) )
	   mErrRet(emsg)
    }

    uiMSG().message( BufferString("Created ", rls.size(),
				  rls.size()>1?" lines":" line") );
    return true;
}


uiGenRanLinesByShift::uiGenRanLinesByShift( uiParent* p )
    : uiDialog( p, Setup("Create Random Lines","Specify generation parameters",
			 "109.0.2") )
    , inctio_(*mMkCtxtIOObj(RandomLineSet))
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt.forread = false;

    infld_ = new uiIOObjSel( this, inctio_, "Input Random Line" );

    const BinID bid1( 1, 1 );
    const BinID bid2( 1 + SI().sampling(false).hrg.step.inl, 1 );
    const Coord c1( SI().transform(bid1) );
    const Coord c2( SI().transform(bid2) );
    distfld_ = new uiGenInput( this, "Distance from input",
				    FloatInpSpec((float)( 4*c1.distTo(c2)) ));
    distfld_->attach( alignedBelow, infld_ );

    const char* strs[] = { "Left", "Right", "Both", 0 };
    sidefld_ = new uiGenInput( this, "Direction", StringListInpSpec(strs) );
    sidefld_->setValue( 2 );
    sidefld_->attach( alignedBelow, distfld_ );

    outfld_ = new uiIOObjSel( this, outctio_, "Output Random line(s)" );
    outfld_->attach( alignedBelow, sidefld_ );
    
    dispfld_ = new uiCheckBox( this, "Display Random Line on creation" );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );
}


uiGenRanLinesByShift::~uiGenRanLinesByShift()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


bool uiGenRanLinesByShift::dispOnCreation()
{
    return dispfld_->isChecked();
}


const char* uiGenRanLinesByShift::getNewSetID() const
{
    BufferString* multid = new BufferString( outctio_.ioobj->key().buf() );
    return outctio_.ioobj ? multid->buf() : 0;
}


bool uiGenRanLinesByShift::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet("Please select the input random line (set)")
    if ( !outfld_->commitInput() )
	mErrRet(outfld_->isEmpty() ?
		"Please select the output random line (set)" : 0)

    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,inctio_.ioobj,msg) )
	mErrRet(msg)

    int lnr = 0;
    if ( inprls.size() > 1 )
    {
	BufferStringSet lnms;
	for ( int idx=0; idx<inprls.size(); idx++ )
	    lnms.add( inprls.lines()[idx]->name() );
	uiSelectFromList dlg( this,
			      uiSelectFromList::Setup("Select one line",lnms) );
	if ( !dlg.go() ) return false;
	lnr = dlg.selection();
    }

    const int choice = sidefld_->getIntValue();
    EM::RandomLineByShiftGenerator gen( inprls, distfld_->getfValue(),
				    choice == 0 ? -1 : (choice == 1 ? 1 : 0) );
    Geometry::RandomLineSet outrls; gen.generate( outrls, lnr );
    if ( outrls.isEmpty() )
	mErrRet("Not enough input points to create output")

    if ( !RandomLineSetTranslator::store(outrls,outctio_.ioobj,msg) )
	mErrRet(msg)

    return true;
}


uiGenRanLineFromPolygon::uiGenRanLineFromPolygon( uiParent* p )
    : uiDialog( p, Setup("Create Random Lines","Specify generation parameters",
			 "109.0.3") )
    , inctio_(*mMkCtxtIOObj(PickSet))
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt.forread = false;
    inctio_.ctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );

    infld_ = new uiIOObjSel( this, inctio_, "Input Polygon" );
    zrgfld_ = new uiSelZRange( this, true );
    zrgfld_->attach( alignedBelow, infld_ );
    outfld_ = new uiIOObjSel( this, outctio_, "Output Random Line" );
    outfld_->attach( alignedBelow, zrgfld_ );
    dispfld_ = new uiCheckBox( this, "Display Random Line on creation" );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );
}


uiGenRanLineFromPolygon::~uiGenRanLineFromPolygon()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


const char* uiGenRanLineFromPolygon::getNewSetID() const
{
    BufferString* multid = new BufferString( outctio_.ioobj->key().buf() );
    return outctio_.ioobj ? multid->buf() : 0;
}


bool uiGenRanLineFromPolygon::dispOnCreation()
{
    return dispfld_->isChecked();
}


bool uiGenRanLineFromPolygon::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet("Please select the input polygon")
    if ( !outfld_->commitInput() )
	mErrRet(outfld_->isEmpty() ?
		"Please select the output random line" : 0)

    PtrMan< ODPolygon<float> > poly = 0;
    BufferString msg;
    poly = PickSetTranslator::getPolygon( *inctio_.ioobj, msg );
    if ( !poly )
       mErrRet(msg)

    Geometry::RandomLine* rl = new Geometry::RandomLine;
    for ( int idx=0; idx<poly->size(); idx++ )
    {
	Geom::Point2D<float> pt = poly->getVertex( idx );
	BinID bid( mNINT32(pt.x), mNINT32(pt.y) );
	rl->addNode( bid );
    }
    if ( rl->isEmpty() )
	{ delete rl; mErrRet("Empty input polygon") }
    rl->setZRange( zrgfld_->getRange() );

    Geometry::RandomLineSet outrls;
    outrls.addLine( rl );
    if ( !RandomLineSetTranslator::store(outrls,outctio_.ioobj,msg) )
	mErrRet(msg)

    return true;
}
