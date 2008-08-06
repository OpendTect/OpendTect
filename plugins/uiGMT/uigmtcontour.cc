/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtcontour.cc,v 1.1 2008-08-06 09:58:05 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtcontour.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "gmtpar.h"
#include "ioobj.h"
#include "linear.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uitaskrunner.h" 


int uiGMTContourGrp::factoryid_ = -1;

void uiGMTContourGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Contour",
				    uiGMTContourGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTContourGrp::createInstance( uiParent* p )
{
    return new uiGMTContourGrp( p );
}


uiGMTContourGrp::uiGMTContourGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Contour")
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , sd_(*new EM::SurfaceIOData)
    , hor_(0)
{
    inpfld_ = new uiIOObjSel( this, ctio_,"Select Horizon" );
    inpfld_->selectiondone.notify( mCB(this,uiGMTContourGrp,objSel) );

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, inpfld_ );
    subselfld_->selChange.notify( mCB(this,uiGMTContourGrp,selChg) );

    readbut_ = new uiPushButton( this, "Read Horizon",
	    			 mCB(this,uiGMTContourGrp,readCB), false );
    readbut_->attach( alignedBelow, subselfld_ );
    readbut_->setSensitive( false );

    rgfld_ = new uiGenInput( this, "Data Range", FloatInpIntervalSpec(true) );
    rgfld_->valuechanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
    rgfld_->attach( alignedBelow, readbut_ );

    nrcontourfld_ = new uiGenInput( this, "Number of contours",
	    			    IntInpSpec() );
    nrcontourfld_->valuechanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
    nrcontourfld_->attach( alignedBelow, rgfld_ );

    resetbut_ = new uiPushButton( this, "Reset range",
	    			  mCB(this,uiGMTContourGrp,resetCB), false );
    resetbut_->attach( rightTo, nrcontourfld_ );
    resetbut_->setSensitive( false );

    linefld_ = new uiCheckBox( this, "Draw contour lines" );
    linefld_->attach( alignedBelow, nrcontourfld_ );
    linefld_->setChecked( true );

    fillfld_ = new uiCheckBox( this, "Fill Color" );
    fillfld_->attach( rightTo, linefld_ );
}


uiGMTContourGrp::~uiGMTContourGrp()
{
    delete &sd_; delete &ctio_;
    if ( hor_ ) hor_->unRef();
}


void uiGMTContourGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput(false) )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) return;

    const char* res = EM::EMM().getSurfaceData( ioobj->key(), sd_ );
    if ( res )
    {
	uiMSG().error( res );
	return;
    }

    CubeSampling cs;
    cs.hrg = sd_.rg;
    subselfld_->setInput( cs );
    resetCB( 0 );
}


void uiGMTContourGrp::resetCB( CallBacker* )
{
    if ( sd_.zrg.isUdf() )
    {
	rgfld_->clear();
	nrcontourfld_->clear();
	readbut_->setSensitive( true );
    }

    AxisLayout zaxis( sd_.zrg );
    StepInterval<float> zrg( sd_.zrg.start, sd_.zrg.stop, zaxis.sd.step/5 );
    rgfld_->setValue( zrg );
    nrcontourfld_->setValue( zrg.nrSteps() + 1 );
    resetbut_->setSensitive( false );
}


void uiGMTContourGrp::selChg( CallBacker* cb )
{
    HorSampling hs = subselfld_->envelope().hrg;
    if ( hs == sd_.rg )
	return;

    readbut_->setSensitive( true );
    resetbut_->setSensitive( false );
}


void uiGMTContourGrp::rgChg( CallBacker* cb )
{
    if ( !cb || !rgfld_ || !nrcontourfld_ ) return;

    mDynamicCastGet(uiGenInput*,fld,cb)
    StepInterval<float> datarg = rgfld_->getFStepInterval();
    if ( fld == rgfld_ )
    {
	int nrcontours = datarg.nrSteps() + 1;
	if ( nrcontours < 2 || nrcontours > 100 )
	{
	    uiMSG().warning( "Step is either too small or too large" );
	    nrcontours = nrcontourfld_->getIntValue();
	    datarg.step = ( datarg.stop - datarg.start ) / ( nrcontours - 1 );
	    rgfld_->setValue( datarg );
	    return;
	}

	nrcontourfld_->setValue( nrcontours );
    }
    else if ( fld == nrcontourfld_ )
    {
	int nrcontours = nrcontourfld_->getIntValue();
	if ( nrcontours < 2 || nrcontours > 100 )
	{
	    uiMSG().warning( "Too many or too few contours" );
	    nrcontours = datarg.nrSteps() + 1;
	    nrcontourfld_->setValue( nrcontours );
	    return;
	}

	datarg.step = ( datarg.stop - datarg.start ) / ( nrcontours - 1 );
	rgfld_->setValue( datarg );
    }

    if ( !sd_.zrg.isUdf() )
	resetbut_->setSensitive( true );
}


void uiGMTContourGrp::readCB( CallBacker* )
{
    if ( !inpfld_->commitInput(false) )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) return;

    if ( ( !hor_ || hor_->multiID()!=ioobj->key() ) && !loadHor() )
	return;

    Interval<float> rg( SI().zRange(false).stop, SI().zRange(false).start );
    HorSamplingIterator iter( subselfld_->envelope().hrg );
    BinID bid;
    EM::SectionID sid = hor_->sectionID( 0 );
    while ( iter.next(bid) )
    {
	EM::SubID subid = bid.getSerialized();
	Coord3 pos = hor_->getPos(sid,subid);
	if ( pos.isDefined() )
	    rg.include( pos.z, false );
    }

    sd_.zrg = rg;
    readbut_->setSensitive( false );
    resetCB( 0 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTContourGrp::loadHor()
{
    if ( hor_ )
	hor_->unRef();

    IOObj* ioobj = ctio_.ioobj;
    EM::EMObject* obj = 0;
    EM::ObjectID id = EM::EMM().getObjectID( ioobj->key() );
    if ( id < 0 )
    {
	EM::SurfaceIOData sd;
	sd.rg = subselfld_->envelope().hrg;
	PtrMan<EM::SurfaceIODataSelection> sel =
	    				new EM::SurfaceIODataSelection( sd );
	PtrMan<Executor> exec = EM::EMM().objectLoader( ioobj->key(), sel );
	if ( !exec )
	    return false;

	uiTaskRunner dlg( this );
	if ( !dlg.execute(*exec) )
	    return false;

	id = EM::EMM().getObjectID( ioobj->key() );
	obj = EM::EMM().getObject( id );
	obj->ref();
    }
    else
    {
	obj = EM::EMM().getObject( id );
	obj->ref();
    }

    mDynamicCastGet(EM::Horizon3D*,hor3d,obj)
    if ( !hor3d )
	return false;

    hor_ = hor3d;
    hor_->ref();
    return true;
}


bool uiGMTContourGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput(false) || !ctio_.ioobj )
	mErrRet("Please select a Horizon")

    inpfld_->fillPar( par );
    par.set( sKey::Name, ctio_.ioobj->name() );
    IOPar subpar;
    subselfld_->fillPar( subpar );
    par.mergeComp( subpar, sKey::Selection );
    StepInterval<float> rg = rgfld_->getFStepInterval();
    par.set( ODGMT::sKeyDataRange, rg );
    const bool drawcontour = linefld_->isChecked();
    const bool dofill = fillfld_->isChecked();
    if ( !drawcontour && !dofill )
	mErrRet("Check at least one of the drawing options")

    par.setYN( ODGMT::sKeyDrawContour, drawcontour );
    par.setYN( ODGMT::sKeyFill, dofill );
    return true;
}


bool uiGMTContourGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    PtrMan<IOPar> subpar = par.subselect( sKey::Selection );
    if ( !subpar )
	return false;

    subselfld_->usePar( *subpar );
    StepInterval<float> rg;
    par.get( ODGMT::sKeyDataRange, rg );
    rgfld_->setValue( rg );
    nrcontourfld_->setValue( rg.nrSteps() + 1 );
    bool drawcontour, dofill;
    par.getYN( ODGMT::sKeyDrawContour, drawcontour );
    par.getYN( ODGMT::sKeyFill, dofill );
    linefld_->setChecked( drawcontour );
    fillfld_->setChecked( dofill );
    return true;
}
