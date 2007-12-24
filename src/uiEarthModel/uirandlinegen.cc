/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uirandlinegen.cc,v 1.7 2007-12-24 16:51:22 cvsbert Exp $
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
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiGenRanLinesByContour::uiGenRanLinesByContour( uiParent* p )
    : uiDialog( p, Setup("Create Random Lines","Specify generation parameters",
			 "0.0.0") )
    , horctio_(*mGetCtxtIOObj(EMHorizon3D,Surf))
    , polyctio_(*mMkCtxtIOObj(PickSet))
    , rlsctio_(*mMkCtxtIOObj(RandomLineSet))
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    horctio_.setObj( IOM().getIfOnlyOne(mTranslGroupName(EMHorizon3D)) );
    rlsctio_.ctxt.forread = false;
    polyctio_.ctxt.parconstraints.set( sKey::Type, sKey::Polygon );
    polyctio_.ctxt.allowcnstrsabsent = false;
    polyctio_.fillIfOnlyOne( IOObjContext::Loc );

    infld_ = new uiIOObjSel( this, horctio_, "Input Horizon" );
    polyfld_ = new uiIOObjSel( this, polyctio_, "Within polygon (optional)" );
    polyfld_->attach( alignedBelow, infld_ );

    StepInterval<float> sizrg( SI().zRange(true) );
    sizrg.scale( SI().zFactor() );
    StepInterval<float> suggestedzrg( sizrg );
    suggestedzrg.step *= 10;
    contzrgfld_ = new uiGenInput( this, "Contour Z range",
			FloatInpIntervalSpec(suggestedzrg) );
    contzrgfld_->attach( alignedBelow, polyfld_ );

    static const char* fldnm = "Random line Z range";
    const float wdth = 50 * sizrg.step;
    relzrgfld_ = new uiGenInput( this, fldnm,
			FloatInpIntervalSpec(Interval<float>(-wdth,wdth)) );
    relzrgfld_->attach( alignedBelow, contzrgfld_ );
    Interval<float> abszrg; assign( abszrg, sizrg );
    abszrgfld_ = new uiGenInput( this, fldnm, FloatInpIntervalSpec(abszrg) );
    abszrgfld_->attach( alignedBelow, contzrgfld_ );
    const CallBack cb( mCB(this,uiGenRanLinesByContour,isrelChg) );
    isrelfld_ = new uiCheckBox( this, "Relative to horizon", cb );
    isrelfld_->setChecked( true );
    isrelfld_->attach( rightOf, abszrgfld_ );

    outfld_ = new uiIOObjSel( this, rlsctio_, "Random Line set" );
    outfld_->attach( alignedBelow, relzrgfld_ );

    finaliseDone.notify( cb );
}


uiGenRanLinesByContour::~uiGenRanLinesByContour()
{
    delete horctio_.ioobj; delete &horctio_;
    delete polyctio_.ioobj; delete &polyctio_;
    delete rlsctio_.ioobj; delete &rlsctio_;
}


const char* uiGenRanLinesByContour::getNewSetID() const
{
    return rlsctio_.ioobj ? rlsctio_.ioobj->key().buf() : 0;
}


void uiGenRanLinesByContour::isrelChg( CallBacker* )
{
    const bool isrel = isrelfld_->isChecked();
    relzrgfld_->display( isrel );
    abszrgfld_->display( !isrel );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGenRanLinesByContour::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput(false) )
	mErrRet("Please select the input horizon")
    if ( !outfld_->commitInput(true) )
	mErrRet("Please select the output random line set")

    polyfld_->commitInput( false );
    PtrMan< ODPolygon<float> > poly = 0;
    if ( polyctio_.ioobj )
    {
	BufferString msg;
	poly = PickSetTranslator::getPolygon( *polyctio_.ioobj, msg );
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
    const float zfac = 1 / SI().zFactor();
    contzrg.scale( zfac ); linezrg.scale( zfac );

    EM::RandomLineSetByContourGenerator::Setup setup( isrel );
    setup.contzrg( contzrg ).linezrg( linezrg );
    if ( poly ) setup.selpoly_ = poly;
    EM::RandomLineSetByContourGenerator gen( *hor, setup );
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
			 "0.0.0") )
    , inctio_(*mGetCtxtIOObj(RandomLineSet,Loc))
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt.forread = false;

    infld_ = new uiIOObjSel( this, inctio_, "Input Random Line" );

    const BinID bid1( 1, 1 );
    const BinID bid2( 1 + SI().sampling(false).hrg.step.inl, 1 );
    const Coord c1( SI().transform(bid1) );
    const Coord c2( SI().transform(bid2) );
    distfld_ = new uiGenInput( this, "Distance from input",
				    FloatInpSpec(4*c1.distTo(c2)) );
    distfld_->attach( alignedBelow, infld_ );

    const char* strs[] = { "Left", "Right", "Both", 0 };
    sidefld_ = new uiGenInput( this, "Direction", StringListInpSpec(strs) );
    sidefld_->setValue( 2 );
    sidefld_->attach( alignedBelow, distfld_ );

    outfld_ = new uiIOObjSel( this, outctio_, "Output Random line(s)" );
    outfld_->attach( alignedBelow, sidefld_ );
}


uiGenRanLinesByShift::~uiGenRanLinesByShift()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


const char* uiGenRanLinesByShift::getNewSetID() const
{
    return outctio_.ioobj ? outctio_.ioobj->key().buf() : 0;
}


bool uiGenRanLinesByShift::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput(false) )
	mErrRet("Please select the input random line (set)")
    if ( !outfld_->commitInput(true) )
	mErrRet("Please select the output random line (set)")

    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,inctio_.ioobj,msg) )
	mErrRet(msg)

    const int choice = sidefld_->getIntValue();
    EM::RandomLineByShiftGenerator gen( inprls, distfld_->getfValue(),
				    choice == 0 ? -1 : (choice == 1 ? 1 : 0) );
    Geometry::RandomLineSet outrls; gen.generate( outrls );
    if ( outrls.isEmpty() )
	mErrRet("Not (fully) implemented yet")

    if ( !RandomLineSetTranslator::store(outrls,outctio_.ioobj,msg) )
	mErrRet(msg)

    return true;
}


uiGenRanLineFromPolygon::uiGenRanLineFromPolygon( uiParent* p )
    : uiDialog( p, Setup("Create Random Lines","Specify generation parameters",
			 "0.0.0") )
    , inctio_(*mMkCtxtIOObj(PickSet))
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt.forread = false;
    inctio_.ctxt.parconstraints.set( sKey::Type, sKey::Polygon );
    inctio_.ctxt.allowcnstrsabsent = false;
    inctio_.fillIfOnlyOne( IOObjContext::Loc );

    infld_ = new uiIOObjSel( this, inctio_, "Input Polygon" );
    Interval<float> zrg; assign( zrg, SI().zRange(true) );
    zrgfld_ = new uiGenInput( this, "Z Range", FloatInpIntervalSpec(zrg) );
    zrgfld_->attach( alignedBelow, infld_ );
    outfld_ = new uiIOObjSel( this, outctio_, "Output Random Line" );
    outfld_->attach( alignedBelow, zrgfld_ );
}


uiGenRanLineFromPolygon::~uiGenRanLineFromPolygon()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


const char* uiGenRanLineFromPolygon::getNewSetID() const
{
    return outctio_.ioobj ? outctio_.ioobj->key().buf() : 0;
}


bool uiGenRanLineFromPolygon::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput(false) )
	mErrRet("Please select the input polygon")
    if ( !outfld_->commitInput(true) )
	mErrRet("Please select the output random line")

    PtrMan< ODPolygon<float> > poly = 0;
    BufferString msg;
    poly = PickSetTranslator::getPolygon( *inctio_.ioobj, msg );
    if ( !poly )
       mErrRet(msg)

    Geometry::RandomLine* rl = new Geometry::RandomLine;
    for ( int idx=0; idx<poly->size(); idx++ )
    {
	Geom::Point2D<float> pt = poly->getVertex( idx );
	BinID bid( mNINT(pt.x), mNINT(pt.y) );
	rl->addNode( bid );
    }
    if ( rl->isEmpty() )
	{ delete rl; mErrRet("Empty input polygon") }
    rl->setZRange( zrgfld_->getFInterval() );

    Geometry::RandomLineSet outrls;
    outrls.addLine( rl );
    if ( !RandomLineSetTranslator::store(outrls,outctio_.ioobj,msg) )
	mErrRet(msg)

    return true;
}
