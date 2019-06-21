/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          January 2007
________________________________________________________________________

-*/

#include "uirandlinegen.h"

#include "ioobjctxt.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emrandlinegen.h"
#include "emsurfacetr.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survinfo.h"
#include "pickset.h"
#include "polygon.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiselsurvranges.h"
#include "uipicksetsel.h"
#include "uimsg.h"
#include "uitaskrunnerprovider.h"
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
    infld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D) );
    uiIOObjSel::Setup osu( tr("Within polygon") ); osu.optional( true );
    polyfld_ = new uiPickSetIOObjSel( this, osu, true,
				      uiPickSetIOObjSel::PolygonOnly );
    polyfld_->attach( alignedBelow, infld_ );

    StepInterval<float> sizrg( SI().zRange( OD::UsrWork ) );
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

    IOObjContext rlsctxt( mIOObjContext(RandomLineSet) );
    rlsctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, rlsctxt, tr("Random Line set") );
    outfld_->attach( alignedBelow, relzrgfld_ );

    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );

    postFinalise().notify( cb );
}


bool uiGenRanLinesByContour::dispOnCreation()
{
    return dispfld_->isChecked();
}


DBKey uiGenRanLinesByContour::getNewSetID() const
{
    const IOObj* ioobj = outfld_->ioobj( true );
    return ioobj ? ioobj->key() : DBKey::getInvalid();
}


void uiGenRanLinesByContour::largestOnlyChg( CallBacker* )
{
    nrlargestfld_->setSensitive( largestfld_->isChecked() );
}


void uiGenRanLinesByContour::isrelChg( CallBacker* )
{
    const bool isrel = isrelfld_->isChecked();
    relzrgfld_->display( isrel );
    abszrgfld_->display( !isrel );
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiGenRanLinesByContour::acceptOK()
{
    const IOObj* horioobj = infld_->ioobj();
    if ( !horioobj )
	return false;
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    PtrMan< ODPolygon<float> > poly = 0;
    if ( polyfld_->isChecked() )
    {
	poly = polyfld_->getSelectionPolygon();
	if ( !poly )
	    return false;
    }

    uiTaskRunnerProvider trprov( this );
    EM::ObjectManager& mgr = EM::Hor3DMan();
    EM::Object* emobj = mgr.loadIfNotFullyLoaded( horioobj->key(), trprov );
    mDynamicCastGet( EM::Horizon3D*, hor, emobj )
    if ( !hor )
	return false;
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
	uiString emsg;
	if ( !RandomLineSetTranslator::store(rls,outioobj,emsg) )
	   mErrRet(emsg)
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
    IOObjContext ctxt( mIOObjContext(RandomLineSet) );
    infld_ = new uiIOObjSel( this, ctxt,
			     uiStrings::phrInput(uiStrings::sRandomLine()));

    const BinID bid1( 1, 1 );
    const BinID bid2( 1 + SI().inlStep(), 1 );
    const Coord c1( SI().transform(bid1) );
    const Coord c2( SI().transform(bid2) );
    distfld_ = new uiGenInput( this, tr("Distance from input"),
				    FloatInpSpec( 4*c1.distTo<float>(c2) ));
    distfld_->attach( alignedBelow, infld_ );

    uiStringSet dirs;
    dirs.add( uiStrings::sLeft() )
	.add( uiStrings::sRight() )
	.add( uiStrings::sBoth() );
    sidefld_ = new uiGenInput(this, uiStrings::sDirection(),
			      StringListInpSpec(dirs));
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


bool uiGenRanLinesByShift::dispOnCreation()
{
    return dispfld_->isChecked();
}


DBKey uiGenRanLinesByShift::getNewSetID() const
{
    const IOObj* ioobj = outfld_->ioobj( true );
    return ioobj ? ioobj->key() : DBKey::getInvalid();
}


bool uiGenRanLinesByShift::acceptOK()
{
    const IOObj* inioobj = infld_->ioobj();
    if ( !inioobj )
	return false;
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    Geometry::RandomLineSet inprls; uiString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,inioobj,msg) )
	mErrRet(msg)

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

    if ( !RandomLineSetTranslator::store(outrls,outioobj,msg) )
	mErrRet(msg)

    return true;
}


uiGenRanLineFromPolygon::uiGenRanLineFromPolygon( uiParent* p )
    : uiDialog( p, Setup(uiGenRanLinesByContour::sDlgTitle(),
			 uiGenRanLinesByContour::sSpecGenPar(),
			 mODHelpKey(mGenRanLinesFromPolygonHelpID) ) )
{
    infld_ = new uiPickSetIOObjSel( this, true, uiPickSetIOObjSel::PolygonOnly);

    zrgfld_ = new uiSelZRange( this, true );
    zrgfld_->attach( alignedBelow, infld_ );

    IOObjContext rlsctxt( mIOObjContext(RandomLineSet) );
    rlsctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, rlsctxt );
    outfld_->attach( alignedBelow, zrgfld_ );

    dispfld_ = new uiCheckBox( this, tr("Display Random Line on creation") );
    dispfld_->attach( alignedBelow, outfld_ );
    dispfld_->setChecked( true );
}


DBKey uiGenRanLineFromPolygon::getNewSetID() const
{
    const IOObj* ioobj = outfld_->ioobj( true );
    return ioobj ? ioobj->key() : DBKey::getInvalid();
}


bool uiGenRanLineFromPolygon::dispOnCreation()
{
    return dispfld_->isChecked();
}


bool uiGenRanLineFromPolygon::acceptOK()
{
    PtrMan< ODPolygon<float> > poly = infld_->getSelectionPolygon();
    if ( !poly )
       return false;
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    RefMan<Geometry::RandomLine> rl = new Geometry::RandomLine;
    for ( int idx=0; idx<poly->size(); idx++ )
    {
	Geom::Point2D<float> pt = poly->getVertex( idx );
	BinID bid( mNINT32(pt.x_), mNINT32(pt.y_) );
	rl->addNode( bid );
    }
    if ( rl->isEmpty() )
	{ mErrRet(tr("Empty input polygon")) }
    rl->setZRange( zrgfld_->getRange() );

    Geometry::RandomLineSet outrls;
    outrls.addLine( *rl );
    uiString msg;
    if ( !RandomLineSetTranslator::store(outrls,outioobj,msg) )
	mErrRet(msg)

    return true;
}
