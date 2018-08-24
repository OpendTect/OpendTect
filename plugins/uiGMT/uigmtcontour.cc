/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "uigmtcontour.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicolseqsel.h"
#include "uicolsequsemodesel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uisellinest.h"
#include "uitaskrunner.h"

#include "axislayout.h"
#include "coltabsequence.h"
#include "ioobjctxt.h"
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
    , sd_(*new EM::SurfaceIOData)
    , hor_(0)
    , lsfld_(0)
{
    inpfld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D),
			      uiStrings::sHorizon() );
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
    rgfld_->valuechanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
    rgfld_->attach( alignedBelow, lcb );

    nrcontourfld_ = new uiGenInput( this, tr("Number of contours"),
				    IntInpSpec() );
    nrcontourfld_->valuechanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
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

    colseqfld_ = new uiColSeqSel( this, OD::Horizontal,
				    uiStrings::sColorTable() );
    colseqfld_->attach( rightOf, fillfld_ );

    sequsefld_ = new uiColSeqUseModeSel( this, true, uiString::empty() );
    sequsefld_->attach( rightOf, colseqfld_ );

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
    sequsefld_->setSensitive( dofill );
}


void uiGMTContourGrp::objSel( CallBacker* )
{
    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return;

    EM::IOObjInfo eminfo( ioobj->key() );
    if ( !eminfo.isOK() )
	{ uiMSG().error( ioobj->phrCannotReadObj() ); return; }

    uiString errmsg;
    if ( !eminfo.getSurfaceData(sd_,errmsg) )
	return;

    TrcKeyZSampling cs;
    cs.hsamp_ = sd_.rg;
    subselfld_->setInput( cs );
    attribfld_->setEmpty();
    attribfld_->addItem( toUiString(ODGMT::sKeyZVals()) );

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


void uiGMTContourGrp::selChg( CallBacker* cb )
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
    rgfld_->valuechanged.disable();
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
	    rgfld_->valuechanged.enable();
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
	    rgfld_->valuechanged.enable();
	    return;
	}

	datarg.step = ( datarg.stop - datarg.start ) / ( nrcontours - 1 );
	rgfld_->setValue( datarg );
    }

    rgfld_->valuechanged.enable();
    if ( !valrg_.isUdf() )
	resetbut_->setSensitive( true );
}


void uiGMTContourGrp::readCB( CallBacker* )
{
    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return;

    TrcKeySampling hs = subselfld_->envelope().hsamp_;
    if ( ( !hor_ || hor_->dbKey()!=ioobj->key() ) && !loadHor() )
	return;

    BufferString attrnm = attribfld_->itemText( attribfld_->currentItem() );
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
    do
    {
	const EM::PosID posid = EM::PosID::getFromRowCol( iter.curBinID() );
	const float val = isz ? (float) hor_->getPos( posid ).z_
			      : hor_->auxdata.getAuxDataVal( dataidx, posid );
	if ( !mIsUdf(val) )
	    rg.include( val, false );
    } while ( iter.next() );

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

    uiTaskRunnerProvider trprov( this );
    ConstRefMan<EM::Object> obj = EM::MGR().fetch( ioobj->key(), trprov );
    if ( !obj )
	return false;

    mDynamicCastGet(const EM::Horizon3D*,hor3d,obj.ptr())
    if ( !hor3d )
	return false;

    hor_ = hor3d;
    hor_->ref();
    return true;
}


bool uiGMTContourGrp::fillPar( IOPar& par ) const
{
    const IOObj* ioobj = inpfld_->ioobj();
    if ( !ioobj ) return false;

    inpfld_->fillPar( par );
    par.set( sKey::Name(), ioobj->name() );
    const int attribidx = attribfld_->currentItem();
    par.set( ODGMT::sKeyAttribName(), attribfld_->itemText(attribidx) );
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
    par.set( ODGMT::sKeyColSeq(), colseqfld_->seqName() );
    const ColTab::SeqUseMode usemode = sequsefld_->mode();
    ColTab::toPar( usemode, par );

    return true;
}


bool uiGMTContourGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const char* attribname = par.find( ODGMT::sKeyAttribName() );
    if ( attribname && *attribname )
	attribfld_->setCurrentItem( attribname );

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
	FixedString lskey = par.find( ODGMT::sKeyLineStyle() );
	OD::LineStyle ls; ls.fromString( lskey.str() );
	lsfld_->setStyle( ls );
    }

    fillfld_->setChecked( dofill );
    if ( dofill )
    {
	colseqfld_->setSeqName( par.find(ODGMT::sKeyColSeq()) );
	ColTab::SeqUseMode usemode = ColTab::UnflippedSingle;
	ColTab::fromPar( par, usemode );
	sequsefld_->setMode( usemode );
    }

    drawSel( 0 );
    return true;
}
