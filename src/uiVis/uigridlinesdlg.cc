/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigridlinesdlg.h"

#include "draw.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uisellinest.h"
#include "visgridlines.h"
#include "visplanedatadisplay.h"
#include <math.h>


#define mCreateGridFld( name, lbl ) \
    label = "Show"; label += " "; label += lbl; label += " "; \
    label += "grid"; \
    name##fld_ = new uiCheckBox( this, label ); \
    name##fld_->activated.notify( mCB(this,uiGridLinesDlg,showGridLineCB) ); \
    name##spacingfld_ = new uiGenInput( this, "Spacing (Start/Stop)", \
	    				IntInpIntervalSpec(true) ); \
    name##spacingfld_->attach( leftAlignedBelow, name##fld_ );

    
uiGridLinesDlg::uiGridLinesDlg( uiParent* p, visSurvey::PlaneDataDisplay* pdd )
    : uiDialog(p,uiDialog::Setup("GridLines","Set gridlines options","50.0.3"))
    , pdd_( pdd )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , zfld_( 0 )
    , inlspacingfld_( 0 )
    , crlspacingfld_( 0 )
    , zspacingfld_( 0 )
{
    BufferString label;
    CubeSampling cs( pdd->getCubeSampling(true,true) );
    if ( cs.nrInl()>1 ) 
	{ mCreateGridFld( inl, "inline" ) }
    if ( cs.nrCrl()>1 )
    	{ mCreateGridFld( crl, "crossline" ) }
    if ( cs.nrZ()>1 )
    	{ mCreateGridFld( z, "z" ) }

    if ( inlfld_ && crlfld_ )
	crlfld_->attach( leftAlignedBelow, inlspacingfld_ );

    LineStyle lst;
    pdd->gridlines()->getLineStyle( lst );
    
    lsfld_ = new uiSelLineStyle( this, lst, "Line style" );
    if ( zfld_ )
    {
	zfld_->attach( leftAlignedBelow, inlfld_ ? inlspacingfld_ 
						 : crlspacingfld_ );
	lsfld_->attach( alignedBelow, zspacingfld_ );
    }
    else
	lsfld_->attach( alignedBelow, crlspacingfld_ );

    BufferString allmsg("Apply to all loaded ");
    if ( visSurvey::PlaneDataDisplay::Inline == pdd_->getOrientation() )
	allmsg += "inlines";
    else if ( visSurvey::PlaneDataDisplay::Crossline == pdd_->getOrientation() )
	allmsg += "crosslines";
    else
	allmsg += "z slices";
    applyallfld_ = new uiCheckBox( this, allmsg.buf() );
    applyallfld_->setChecked( true );
    applyallfld_->attach( alignedBelow, lsfld_ );
        
    setParameters();
}


void uiGridLinesDlg::showGridLineCB( CallBacker* cb )
{
    if ( inlspacingfld_ )
	inlspacingfld_->display( inlfld_->isChecked() );
    
    if ( crlspacingfld_ )
	crlspacingfld_->display( crlfld_->isChecked() );
    
    if ( zspacingfld_ )
	zspacingfld_->display( zfld_->isChecked() );
    
    lsfld_->display( (inlfld_ && inlfld_->isChecked()) ||
	    	     (crlfld_ && crlfld_->isChecked()) ||
		     (zfld_ && zfld_->isChecked()) );
}


static float getDefaultStep( float width )
{
    float reqstep = width / 5;
    float step = 10000;
    while ( true )
    {
	if ( step <= reqstep )
	    return step;
	step /= 10;
    }
}


static void getDefaultHorSampling( int& start, int& stop, int& step )
{
    const float width = mCast( float, stop - start );
    step = mNINT32( getDefaultStep(width) );

    start = step * (int)( ceil( (float)start/(float)step ) );
    stop = step * (int)( floor( (float)stop/(float)step ) );
}


static void getDefaultZSampling( StepInterval<float>& zrg )
{
    const float width = (zrg.stop-zrg.start) * SI().zDomain().userFactor();
    zrg.step = getDefaultStep( width );
    zrg.start = zrg.step * 
	ceil( (float)zrg.start * SI().zDomain().userFactor() /(float)zrg.step );
    zrg.stop = zrg.step * 
	floor( (float)zrg.stop * SI().zDomain().userFactor() /(float)zrg.step );
}


void uiGridLinesDlg::setParameters()
{
    const bool hasgl = !pdd_->gridlines()->getGridCubeSampling().isEmpty();
    CubeSampling cs = pdd_->getCubeSampling( true, true );
    if ( hasgl )
    {
	cs = pdd_->gridlines()->getGridCubeSampling();
	cs.zrg.scale( mCast(float,SI().zDomain().userFactor()) );
    }
    else
    {
	getDefaultHorSampling( cs.hrg.start.inl, cs.hrg.stop.inl, 
			       cs.hrg.step.inl );
	getDefaultHorSampling( cs.hrg.start.crl, cs.hrg.stop.crl, 
			       cs.hrg.step.crl );
	getDefaultZSampling( cs.zrg );
    }
    
    if ( inlfld_ )
    {
	inlfld_->setChecked( pdd_->gridlines()->areInlinesShown() );
	inlspacingfld_->setValue( cs.hrg.inlRange() );
    }

    if ( crlfld_ )
    {
	crlfld_->setChecked( pdd_->gridlines()->areCrosslinesShown() );
	crlspacingfld_->setValue( cs.hrg.crlRange() );
    }

    if ( zfld_ )
    {
	zfld_->setChecked( pdd_->gridlines()->areZlinesShown() );
	zspacingfld_->setValue(
		StepInterval<int>(mNINT32(cs.zrg.start),mNINT32(cs.zrg.stop),
		   		  mNINT32(cs.zrg.step)) );
    }

    showGridLineCB(0);
}


#define mGetHrgSampling(dir)\
    StepInterval<int> dir##intv = dir##spacingfld_->getIStepInterval();\
    cs.hrg.start.dir = dir##intv.start;\
    cs.hrg.stop.dir = dir##intv.stop;\
    cs.hrg.step.dir = dir##intv.step;\


bool uiGridLinesDlg::acceptOK( CallBacker* )
{
    CubeSampling cs;
    if ( inlfld_ ) { mGetHrgSampling(inl) };
    if ( crlfld_ ) { mGetHrgSampling(crl) };
    if ( zfld_ )
    {
	cs.zrg.setFrom( zspacingfld_->getFStepInterval() );
	cs.zrg.scale( 1.f/SI().zDomain().userFactor() );
    }

    if ( (inlfld_ && inlfld_->isChecked() && cs.hrg.step.inl==0) ||
	 (crlfld_ && crlfld_->isChecked() && cs.hrg.step.crl==0) ||
	 (zfld_ && zfld_->isChecked() && mIsZero(cs.zrg.step,mDefEps)) )
    {
	uiMSG().error( "Please make sure all steps are non-zero" );
	return false;
    }

    visSurvey::Scene* scene = pdd_->getScene();
    const bool applyall = applyallfld_->isChecked();
    for ( int idx=scene->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(visBase::VisualObject*,so,scene->getObject(idx));
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,so);
	if ( !pdd || pdd->getOrientation()!=pdd_->getOrientation() )
	    continue;

	if ( !applyall && pdd!=pdd_ )
	    continue;
	
	visBase::GridLines& gl = *pdd->gridlines();
	gl.setPlaneCubeSampling( pdd->getCubeSampling(true,true) );
	gl.setGridCubeSampling( cs );
	gl.showInlines( inlfld_ ? inlfld_->isChecked() : false );
	gl.showCrosslines( crlfld_ ? crlfld_->isChecked(): false );
	gl.showZlines( zfld_ ? zfld_->isChecked(): false );
	gl.setLineStyle( lsfld_->getStyle() );
    }

    return true;
}


