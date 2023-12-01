/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
#include "uiiosurface.h"
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
{
    infld_ = new uiHorizonSel( this, false, true );

    IOObjContext polyctxt = mIOObjContext( PickSet );
    PickSetTranslator::fillConstraints( polyctxt, true );
    uiIOObjSel::Setup osu( tr("Within polygon") ); osu.optional( true );
    polyfld_ = new uiIOObjSel( this, polyctxt, osu );
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

    IOObjContext rlctxt = mIOObjContext( RandomLineSet );
    rlctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, rlctxt, tr("Random Line set") );
    outfld_->attach( alignedBelow, relzrgfld_ );

    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );

    postFinalize().notify( cb );
}


uiGenRanLinesByContour::~uiGenRanLinesByContour()
{
}


bool uiGenRanLinesByContour::dispOnCreation()
{
    return dispfld_->isChecked();
}


MultiID uiGenRanLinesByContour::getNewSetID() const
{
    return outfld_->key( true );
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
    const IOObj* horioobj = infld_->ioobj();
    const IOObj* rlioobj = outfld_->ioobj();
    if ( !horioobj || !rlioobj )
	return false;

    PtrMan< ODPolygon<float> > poly = nullptr;
    if ( polyfld_->isChecked() )
    {
	const IOObj* polyioobj = polyfld_->ioobj();
	if ( !polyioobj )
	    return false;

	BufferString msg;
	poly = PickSetTranslator::getPolygon( *polyioobj, msg );
	if ( !poly )
	   mErrRet(mToUiStringTodo(msg))
    }

    uiTaskRunner taskrunner( this );
    RefMan<EM::EMObject> emobj =
	EM::EMM().loadIfNotFullyLoaded( horioobj->key(), &taskrunner );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
    if ( !hor )
	return false;

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

    const int rlssz = rls.size();
    if ( rlssz < 1 )
       mErrRet(uiStrings::phrCannotFind(tr("a contour in range")))
    else
    {
	BufferString emsg;
	if ( !RandomLineSetTranslator::store(rls,rlioobj,emsg) )
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
{
    IOObjContext ctxt = mIOObjContext( RandomLineSet );
    infld_ = new uiIOObjSel( this, ctxt,
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

    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrOutput(uiStrings::sRandomLine()));
    outfld_->attach( alignedBelow, sidefld_ );

    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );
}


uiGenRanLinesByShift::~uiGenRanLinesByShift()
{
}


bool uiGenRanLinesByShift::dispOnCreation()
{
    return dispfld_->isChecked();
}


MultiID uiGenRanLinesByShift::getNewSetID() const
{
    return outfld_->key();
}


bool uiGenRanLinesByShift::acceptOK( CallBacker* )
{
    const IOObj* ioobjin = infld_->ioobj();
    if ( !ioobjin )
	return false;

    const IOObj* ioobjout = outfld_->ioobj();
    if ( !ioobjout )
	return false;

    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,ioobjin,msg) )
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

    if ( !RandomLineSetTranslator::store(outrls,ioobjout,msg) )
	mErrRet(mToUiStringTodo(msg))

    return true;
}


uiGenRanLineFromPolygon::uiGenRanLineFromPolygon( uiParent* p )
    : uiDialog( p, Setup(uiGenRanLinesByContour::sDlgTitle(),
			 uiGenRanLinesByContour::sSpecGenPar(),
			 mODHelpKey(mGenRanLinesFromPolygonHelpID) ) )
{
    IOObjContext psctxt = mIOObjContext( PickSet );
    PickSetTranslator::fillConstraints( psctxt, true );
    infld_ = new uiIOObjSel( this, psctxt,
			     uiStrings::phrInput(uiStrings::sPolygon()) );

    zrgfld_ = new uiSelZRange( this, true );
    zrgfld_->attach( alignedBelow, infld_ );

    IOObjContext rlctxt = mIOObjContext( RandomLineSet );
    rlctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, rlctxt,
			      uiStrings::phrOutput(uiStrings::sRandomLine()) );
    outfld_->attach( alignedBelow, zrgfld_ );

    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );
}


uiGenRanLineFromPolygon::~uiGenRanLineFromPolygon()
{
}


MultiID uiGenRanLineFromPolygon::getNewSetID() const
{
    return outfld_->key( true );
}


bool uiGenRanLineFromPolygon::dispOnCreation()
{
    return dispfld_->isChecked();
}


bool uiGenRanLineFromPolygon::acceptOK( CallBacker* )
{
    const IOObj* psioobj = infld_->ioobj();
    const IOObj* rlioobj = outfld_->ioobj();
    if ( !psioobj || !rlioobj )
	return false;

    PtrMan< ODPolygon<float> > poly = nullptr;
    BufferString msg;
    poly = PickSetTranslator::getPolygon( *psioobj, msg );
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
    if ( !RandomLineSetTranslator::store(outrls,rlioobj,msg) )
	mErrRet(mToUiStringTodo(msg))

    return true;
}
