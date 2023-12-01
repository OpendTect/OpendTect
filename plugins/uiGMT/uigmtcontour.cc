/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtcontour.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uisellinest.h"
#include "uitaskrunner.h"

#include "axislayout.h"
#include "coltabsequence.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "gmtpar.h"
#include "ioobj.h"
#include "survinfo.h"


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
    : uiGMTOverlayGrp(p,uiStrings::sContour())
    , hor_(0)
    , sd_(*new EM::SurfaceIOData)
    , lsfld_(0)
{
    inpfld_ = new uiHorizon3DSel( this, true, uiStrings::sHorizon() );
    inpfld_->selectionDone.notify( mCB(this,uiGMTContourGrp,objSel) );

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, inpfld_ );
    subselfld_->selChange.notify( mCB(this,uiGMTContourGrp,selChg) );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
                                                    uiStrings::sAttribute() );
    attribfld_ = lcb->box();
    attribfld_->selectionChanged.notify( mCB(this,uiGMTContourGrp,readCB) );
    lcb->attach( alignedBelow, subselfld_ );

    uiString ztag = tr("Value range ");
    rgfld_ = new uiGenInput( this, ztag, FloatInpIntervalSpec(true) );
    rgfld_->valueChanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
    rgfld_->attach( alignedBelow, lcb );

    nrcontourfld_ = new uiGenInput( this, tr("Number of contours"),
				    IntInpSpec() );
    nrcontourfld_->valueChanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
    nrcontourfld_->attach( alignedBelow, rgfld_ );

    resetbut_ = new uiPushButton( this, tr("Reset range"),
				  mCB(this,uiGMTContourGrp,resetCB), false );
    resetbut_->attach( rightTo, nrcontourfld_ );
    resetbut_->setSensitive( false );

    linefld_ = new uiCheckBox( this, tr("Draw contour lines"),
			       mCB(this,uiGMTContourGrp,drawSel) );
    linefld_->setChecked( true );
    fillfld_ = new uiCheckBox( this, tr("Fill Color"),
			       mCB(this,uiGMTContourGrp,drawSel) );
    fillfld_->attach( alignedBelow, nrcontourfld_ );
    linefld_->attach( leftOf, fillfld_ );

    colseqfld_ = new uiColorTableSel( this, "Color table" );
    colseqfld_->attach( rightOf, fillfld_ );

    flipfld_ = new uiCheckBox( this, uiStrings::sFlip() );
    flipfld_->attach( rightOf, colseqfld_ );

    lsfld_ = new uiSelLineStyle( this, OD::LineStyle(), tr("Line Style") );
    lsfld_->attach( alignedBelow, fillfld_ );
    drawSel( 0 );
}


uiGMTContourGrp::~uiGMTContourGrp()
{
    delete &sd_;
    if ( hor_ ) hor_->unRef();
}


void uiGMTContourGrp::reset()
{
    if ( hor_ ) hor_->unRef();
    hor_ = 0;
    inpfld_->clear();
    subselfld_->setToAll();
    rgfld_->clear();
    nrcontourfld_->clear();
    linefld_->setChecked( true );
    lsfld_->setStyle( OD::LineStyle() );
    fillfld_->setChecked( false );
    drawSel( 0 );
}


void uiGMTContourGrp::drawSel( CallBacker* )
{
    if ( !lsfld_ ) return;

    lsfld_->setSensitive( linefld_->isChecked() );
    const bool dofill = fillfld_->isChecked();
    colseqfld_->setSensitive( dofill );
    flipfld_->setSensitive( dofill );
    if ( !dofill )
	flipfld_->setChecked( false );
}


void uiGMTContourGrp::objSel( CallBacker* )
{
    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return;

    EM::IOObjInfo eminfo( ioobj->key() );
    if ( !eminfo.isOK() )
    {
	uiString msg = uiStrings::phrCannotRead( ioobj->name() );
	uiMSG().error( msg );
	return;
    }

    uiString errmsg;
    if ( !eminfo.getSurfaceData(sd_,errmsg) )
	return;

    TrcKeyZSampling cs;
    cs.hsamp_ = sd_.rg;
    subselfld_->setInput( cs );
    attribfld_->setEmpty();
    attribfld_->addItem( ODGMT::sKeyZVals() );

    if ( sd_.valnames.isEmpty() )
	attribfld_->setSensitive( false );
    else
    {
	attribfld_->addItems( sd_.valnames );
	attribfld_->setSensitive( true );
    }

    readCB(0);
}


void uiGMTContourGrp::resetCB( CallBacker* )
{
    if ( valrg_.isUdf() )
    {
	rgfld_->clear();
	nrcontourfld_->clear();
    }

    AxisLayout<float> zaxis( valrg_ );
    const StepInterval<float> zrg( valrg_.start, valrg_.stop, zaxis.sd_.step/5);
    rgfld_->setValue( zrg );
    nrcontourfld_->setValue( zrg.nrSteps() + 1 );
    resetbut_->setSensitive( false );
}


void uiGMTContourGrp::selChg( CallBacker* )
{
    TrcKeySampling hs = subselfld_->envelope().hsamp_;
    if ( hs == sd_.rg )
	return;

    readCB(0);
    resetbut_->setSensitive( false );
}


void uiGMTContourGrp::rgChg( CallBacker* cb )
{
    if ( !cb || !rgfld_ || !nrcontourfld_ ) return;

    mDynamicCastGet(uiGenInput*,fld,cb)
    StepInterval<float> datarg = rgfld_->getFStepInterval();
    rgfld_->valueChanged.disable();
    if ( fld == rgfld_ )
    {
	int nrcontours = datarg.nrSteps() + 1;
	if ( nrcontours < 2 || nrcontours > 100 )
	{
	    uiMSG().warning( tr("Step is either too small or too large") );
	    nrcontours = nrcontourfld_->getIntValue();
	    if ( nrcontours == 1 )
		nrcontours = 2;

	    datarg.step = ( datarg.stop - datarg.start ) / ( nrcontours - 1 );
	    rgfld_->setValue( datarg );
	    rgfld_->valueChanged.enable();
	    return;
	}

	nrcontourfld_->setValue( nrcontours );
    }
    else if ( fld == nrcontourfld_ )
    {
	int nrcontours = nrcontourfld_->getIntValue();
	if ( nrcontours < 2 || nrcontours > 100 )
	{
	    uiMSG().warning( tr("Too many or too few contours") );
	    if ( mIsZero(datarg.step,mDefEps) )
		datarg.step = datarg.width();

	    nrcontours = datarg.nrSteps() + 1;
	    nrcontourfld_->setValue( nrcontours );
	    rgfld_->valueChanged.enable();
	    return;
	}

	datarg.step = ( datarg.stop - datarg.start ) / ( nrcontours - 1 );
	rgfld_->setValue( datarg );
    }

    rgfld_->valueChanged.enable();
    if ( !valrg_.isUdf() )
	resetbut_->setSensitive( true );
}


void uiGMTContourGrp::readCB( CallBacker* )
{
    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return;

    TrcKeySampling hs = subselfld_->envelope().hsamp_;
    if ( ( !hor_ || hor_->multiID()!=ioobj->key() ) && !loadHor() )
	return;

    BufferString attrnm = attribfld_->textOfItem( attribfld_->currentItem() );
    const bool isz = attrnm == ODGMT::sKeyZVals();
    int dataidx = -1;
    if ( !isz )
    {
	const int selidx = sd_.valnames.indexOf( attrnm.buf() );
	if ( selidx < 0 ) return;
	PtrMan<Executor> exec = hor_->auxdata.auxDataLoader( selidx );
	if ( exec ) exec->execute();

	dataidx = hor_->auxdata.auxDataIndex( attrnm.buf() );
    }

    Interval<float> rg( mUdf(float), -mUdf(float) );
    TrcKeySamplingIterator iter( hs );
    BinID bid;
    while ( iter.next(bid) )
    {
	const EM::PosID posid( hor_->id(), bid );
	const float val = isz ? (float) hor_->getPos( posid ).z
			      : hor_->auxdata.getAuxDataVal( dataidx, posid );
	if ( !mIsUdf(val) )
	    rg.include( val, false );
    }

    if ( isz )
    {
	rg.scale( mCast(float,SI().zDomain().userFactor()) );
	const float samp = SI().zStep() * SI().zDomain().userFactor();
	rg.start = samp * mNINT32(rg.start/samp);
	rg.stop = samp * mNINT32(rg.stop/samp);
    }

    valrg_ = rg;
    resetCB( 0 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTContourGrp::loadHor()
{
    if ( hor_ )
	hor_->unRef();

    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return false;

    EM::EMObject* obj = 0;
    EM::ObjectID id = EM::EMM().getObjectID( ioobj->key() );
    if ( !id.isValid() || !EM::EMM().getObject(id)->isFullyLoaded() )
    {
	PtrMan<EM::SurfaceIODataSelection> sel =
					new EM::SurfaceIODataSelection( sd_ );
	PtrMan<Executor> exec = EM::EMM().objectLoader( ioobj->key(), sel );
	if ( !exec )
	    return false;

	uiTaskRunner dlg( this );
	if ( !TaskRunner::execute( &dlg, *exec ) )
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
    return true;
}


bool uiGMTContourGrp::fillPar( IOPar& par ) const
{
    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return false;

    inpfld_->fillPar( par );
    par.set( sKey::Name(), ioobj->name() );
    const int attribidx = attribfld_->currentItem();
    par.set( ODGMT::sKeyAttribName(), attribfld_->textOfItem(attribidx) );
    IOPar subpar;
    subselfld_->fillPar( subpar );
    par.mergeComp( subpar, sKey::Selection() );
    StepInterval<float> rg = rgfld_->getFStepInterval();
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) || mIsUdf(rg.step) )
	mErrRet(tr("Invalid data range"))

    par.set( ODGMT::sKeyDataRange(), rg );
    const bool drawcontour = linefld_->isChecked();
    const bool dofill = fillfld_->isChecked();
    if ( !drawcontour && !dofill )
	mErrRet(tr("Check at least one of the drawing options"))

    par.setYN( ODGMT::sKeyDrawContour(), drawcontour );
    if ( drawcontour )
    {
	BufferString lskey;
	lsfld_->getStyle().toString( lskey );
	par.set( ODGMT::sKeyLineStyle(), lskey );
    }

    par.setYN( ODGMT::sKeyFill(), dofill );
    par.set( ODGMT::sKeyColSeq(), colseqfld_->text() );
    par.setYN( ODGMT::sKeyFlipColTab(), flipfld_->isChecked() );

    return true;
}


bool uiGMTContourGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const BufferString attribname = par.find( ODGMT::sKeyAttribName() );
    if ( !attribname.isEmpty() )
	attribfld_->setCurrentItem( attribname.buf() );

    PtrMan<IOPar> subpar = par.subselect( sKey::Selection() );
    if ( !subpar )
	return false;

    subselfld_->usePar( *subpar );
    StepInterval<float> rg;
    par.get( ODGMT::sKeyDataRange(), rg );
    rgfld_->setValue( rg );
    nrcontourfld_->setValue( rg.nrSteps() + 1 );
    bool drawcontour=false, dofill=false;
    par.getYN( ODGMT::sKeyDrawContour(), drawcontour );
    par.getYN( ODGMT::sKeyFill(), dofill );
    linefld_->setChecked( drawcontour );
    if ( drawcontour )
    {
	const BufferString lskey = par.find( ODGMT::sKeyLineStyle() );
	OD::LineStyle ls; ls.fromString( lskey );
	lsfld_->setStyle( ls );
    }

    fillfld_->setChecked( dofill );
    if ( dofill )
    {
	const BufferString colseq( par.find(ODGMT::sKeyColSeq()) );
	colseqfld_->setCurrentItem( colseq.buf() );
	bool doflip = false;
	par.getYN( ODGMT::sKeyFlipColTab(), doflip );
	flipfld_->setChecked( doflip );
    }

    drawSel( 0 );
    return true;
}
