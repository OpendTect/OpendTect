/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtcontour.cc,v 1.24 2012/07/10 13:05:58 cvskris Exp $";

#include "uigmtcontour.h"

#include "coltabsequence.h"
#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "gmtpar.h"
#include "ioobj.h"
#include "axislayout.h"
#include "pixmap.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uisellinest.h"
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
    , lsfld_(0)
{
    inpfld_ = new uiIOObjSel( this, ctio_, "Horizon" );
    inpfld_->selectionDone.notify( mCB(this,uiGMTContourGrp,objSel) );

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, inpfld_ );
    subselfld_->selChange.notify( mCB(this,uiGMTContourGrp,selChg) );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Attribute" );
    attribfld_ = lcb->box();
    attribfld_->selectionChanged.notify( mCB(this,uiGMTContourGrp,readCB) );
    lcb->attach( alignedBelow, subselfld_ );

    BufferString ztag = "Value range ";
    rgfld_ = new uiGenInput( this, ztag, FloatInpIntervalSpec(true) );
    rgfld_->valuechanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
    rgfld_->attach( alignedBelow, lcb );

    nrcontourfld_ = new uiGenInput( this, "Number of contours",
	    			    IntInpSpec() );
    nrcontourfld_->valuechanged.notify( mCB(this,uiGMTContourGrp,rgChg) );
    nrcontourfld_->attach( alignedBelow, rgfld_ );

    resetbut_ = new uiPushButton( this, "Reset range",
	    			  mCB(this,uiGMTContourGrp,resetCB), false );
    resetbut_->attach( rightTo, nrcontourfld_ );
    resetbut_->setSensitive( false );

    linefld_ = new uiCheckBox( this, "Draw contour lines",
	   		       mCB(this,uiGMTContourGrp,drawSel) );
    linefld_->setChecked( true );
    fillfld_ = new uiCheckBox( this, "Fill Color",
	    		       mCB(this,uiGMTContourGrp,drawSel) );
    fillfld_->attach( alignedBelow, nrcontourfld_ );
    linefld_->attach( leftOf, fillfld_ );

    colseqfld_ = new uiComboBox( this, "Col Seq" );
    colseqfld_->attach( rightOf, fillfld_ );
    fillColSeqs();

    flipfld_ = new uiCheckBox( this, "Flip" );
    flipfld_->attach( rightOf, colseqfld_ );

    lsfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    lsfld_->attach( alignedBelow, fillfld_ );
    drawSel( 0 );
}


uiGMTContourGrp::~uiGMTContourGrp()
{
    delete &sd_; delete &ctio_;
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
    lsfld_->setStyle( LineStyle() );
    fillfld_->setChecked( false );
    drawSel( 0 );
}


void uiGMTContourGrp::fillColSeqs()
{
    const int nrseq = ColTab::SM().size();
    for ( int idx=0; idx<nrseq; idx++ )
    {
	const ColTab::Sequence& seq = *ColTab::SM().get( idx );
	colseqfld_->addItem( seq.name() );
	colseqfld_->setPixmap( ioPixmap(seq,16,10,true), idx );
    }
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
    if ( !inpfld_->commitInput() )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) return;

    EM::IOObjInfo eminfo( ioobj->key() );
    if ( !eminfo.isOK() )
    {
	BufferString msg( "Cannot read '", ioobj->name().buf(), "'" );
	uiMSG().error( msg );
	return;
    }

    CubeSampling cs;
    HorSampling emhs;
    emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    cs.hrg = emhs;
    subselfld_->setInput( cs );
    attribfld_->setEmpty();
    attribfld_->addItem( ODGMT::sKeyZVals );

    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    if ( attrnms.size() )
    {
	attribfld_->addItems( attrnms );
	attribfld_->setSensitive( true );	
    }
    else
	attribfld_->setSensitive( false );

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
    const StepInterval<float> zrg( valrg_.start, valrg_.stop, zaxis.sd_.step/5 );
    rgfld_->setValue( zrg );
    nrcontourfld_->setValue( zrg.nrSteps() + 1 );
    resetbut_->setSensitive( false );
}


void uiGMTContourGrp::selChg( CallBacker* cb )
{
    HorSampling hs = subselfld_->envelope().hrg;
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
	    uiMSG().warning( "Step is either too small or too large" );
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
	    uiMSG().warning( "Too many or too few contours" );
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
    if ( !inpfld_->commitInput() )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) return;

    HorSampling hs = subselfld_->envelope().hrg;
    if ( ( !hor_ || hor_->multiID()!=ioobj->key() ) && !loadHor() )
	return;

    BufferString attrnm = attribfld_->textOfItem( attribfld_->currentItem() );
    const bool isz = attrnm == ODGMT::sKeyZVals;
    int dataidx = -1;
    if ( !isz )
    {
	const int selidx = sd_.valnames.indexOf( attrnm.buf() );
	PtrMan<Executor> exec = hor_->auxdata.auxDataLoader( selidx );
	if ( exec ) exec->execute();

	dataidx = hor_->auxdata.auxDataIndex( attrnm.buf() );
    }

    Interval<float> rg( mUdf(float), -mUdf(float) );
    HorSamplingIterator iter( hs );
    BinID bid;
    EM::SectionID sid = hor_->sectionID( 0 );
    while ( iter.next(bid) )
    {
	const EM::PosID posid( hor_->id(), sid, bid.toInt64() );
	const float val = isz ? hor_->getPos( posid ).z
	    		      : hor_->auxdata.getAuxDataVal( dataidx, posid );
	if ( !mIsUdf(val) )
	    rg.include( val, false );
    }

    if ( isz )
    {
	rg.scale( SI().zFactor() );
	const float samp = SI().zStep() * SI().zFactor();
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

    IOObj* ioobj = ctio_.ioobj;
    EM::EMObject* obj = 0;
    EM::ObjectID id = EM::EMM().getObjectID( ioobj->key() );
    if ( id < 0 || !EM::EMM().getObject(id)->isFullyLoaded() )
    {
	PtrMan<EM::SurfaceIODataSelection> sel =
	    				new EM::SurfaceIODataSelection( sd_ );
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
    return true;
}


bool uiGMTContourGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj )
	mErrRet("Please select a Horizon")

    inpfld_->fillPar( par );
    par.set( sKey::Name, ctio_.ioobj->name() );
    const int attribidx = attribfld_->currentItem();
    par.set( ODGMT::sKeyAttribName, attribfld_->textOfItem(attribidx) );
    IOPar subpar;
    subselfld_->fillPar( subpar );
    par.mergeComp( subpar, sKey::Selection );
    StepInterval<float> rg = rgfld_->getFStepInterval();
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) || mIsUdf(rg.step) )
	mErrRet("Invalid data range")

    par.set( ODGMT::sKeyDataRange, rg );
    const bool drawcontour = linefld_->isChecked();
    const bool dofill = fillfld_->isChecked();
    if ( !drawcontour && !dofill )
	mErrRet("Check at least one of the drawing options")

    par.setYN( ODGMT::sKeyDrawContour, drawcontour );
    if ( drawcontour )
    {
	BufferString lskey;
	lsfld_->getStyle().toString( lskey );
	par.set( ODGMT::sKeyLineStyle, lskey );
    }

    par.setYN( ODGMT::sKeyFill, dofill );
    par.set( ODGMT::sKeyColSeq, colseqfld_->text() );
    par.setYN( ODGMT::sKeyFlipColTab, flipfld_->isChecked() );

    return true;
}


bool uiGMTContourGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const char* attribname = par.find( ODGMT::sKeyAttribName );
    if ( attribname && *attribname )
	attribfld_->setCurrentItem( attribname );

    PtrMan<IOPar> subpar = par.subselect( sKey::Selection );
    if ( !subpar )
	return false;

    subselfld_->usePar( *subpar );
    StepInterval<float> rg;
    par.get( ODGMT::sKeyDataRange, rg );
    rgfld_->setValue( rg );
    nrcontourfld_->setValue( rg.nrSteps() + 1 );
    bool drawcontour=false, dofill=false;
    par.getYN( ODGMT::sKeyDrawContour, drawcontour );
    par.getYN( ODGMT::sKeyFill, dofill );
    linefld_->setChecked( drawcontour );
    if ( drawcontour )
    {
	FixedString lskey = par.find( ODGMT::sKeyLineStyle );
	LineStyle ls; ls.fromString( lskey.str() );
	lsfld_->setStyle( ls );
    }

    fillfld_->setChecked( dofill );
    if ( dofill )
    {
	colseqfld_->setCurrentItem( par.find(ODGMT::sKeyColSeq) );
	bool doflip = false;
	par.getYN( ODGMT::sKeyFlipColTab, doflip );
	flipfld_->setChecked( doflip );
    }

    drawSel( 0 );
    return true;
}
