/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uigridlinesdlg.cc,v 1.4 2006-02-28 09:08:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigridlinesdlg.h"
#include "visgridlines.h"
#include "visplanedatadisplay.h"
#include "uigeninput.h"
#include "uisellinest.h"
#include "uibutton.h"
#include "survinfo.h"
#include "draw.h"


#define mCreateGridFld( name, lbl ) \
    label = "Show"; label += " "; label += lbl; label += " ";\
    label += "gridlines";\
    name##fld_ = new uiCheckBox( this, label );\
    name##fld_->activated.notify( mCB(this,uiGridLinesDlg,showLSSel) );\
    name##spacingfld_ = new uiGenInput( this, "Spacing",\
	    				IntInpIntervalSpec( true ) );\
    name##spacingfld_->attach( alignedBelow, name##fld_ );

    
uiGridLinesDlg::uiGridLinesDlg( uiParent* p, visSurvey::PlaneDataDisplay* pdd )
    : uiDialog(p, uiDialog::Setup("GridLines","Set gridlines options") )
    , pdd_( pdd )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , zfld_( 0 )
    , inlspacingfld_( 0 )
    , crlspacingfld_( 0 )
    , zspacingfld_( 0 )
{
    BufferString label;
    CubeSampling cs(pdd->getCubeSampling());
    if ( cs.nrInl()>1 ) 
	{ mCreateGridFld( inl, "inline" ) }
    if ( cs.nrCrl()>1 )
    	{ mCreateGridFld( crl, "crossline" ) }
    if ( cs.nrZ()>1 )
    	{ mCreateGridFld( z, "z" ) }

    if ( inlfld_ && crlfld_ )
	crlfld_->attach( alignedBelow, inlspacingfld_ );

    LineStyle lst;
    pdd->gridlines()->getLineStyle(lst);
    
    lsfld_ = new uiSelLineStyle(this, lst, "Line style", true, true);
    if ( zfld_ )
    {
	zfld_->attach( alignedBelow, inlfld_ ? inlspacingfld_ : crlspacingfld_);
	lsfld_->attach( alignedBelow, zspacingfld_ );
    }
    else
	lsfld_->attach( alignedBelow, crlspacingfld_ );
    
    setParameters();
}


void uiGridLinesDlg::showLSSel( CallBacker* cb )
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


static void setDefaultHorSampling( int& start, int& stop, int& step )
{
    const float interval = stop - start;
    if ( interval/10 < 50 )
	step = 50;
    else if ( interval/10 < 100 )
	step = 100;
    else
	step = 200;

    start = step * (int)( ceil( (float)start/(float)step ) );
    stop = step * (int)( floor( (float)stop/(float)step ) );
}


static void setDefaultZSampling( StepInterval<float>& zrg )
{
    const float interval = ( zrg.stop - zrg.start ) * SI().zFactor();
    zrg.step = interval<1000 ? 100 : 1000;
    zrg.start = zrg.step * 
	ceil( (float)zrg.start * SI().zFactor() / (float)zrg.step );
    zrg.stop = zrg.step * 
	floor( (float)zrg.stop * SI().zFactor() / (float)zrg.step );
}


void uiGridLinesDlg::setParameters()
{
    bool glinited = !pdd_->gridlines()->getGridCubeSampling().isEmpty();
    CubeSampling cs = pdd_->getCubeSampling();
    if ( glinited )
    {
	cs = pdd_->gridlines()->getGridCubeSampling();
	cs.zrg.scale( SI().zFactor() );
    }
    else
    {
	float interval;
	setDefaultHorSampling( cs.hrg.start.inl, cs.hrg.stop.inl, 
			       cs.hrg.step.inl );
	setDefaultHorSampling( cs.hrg.start.crl, cs.hrg.stop.crl, 
			       cs.hrg.step.crl );
	setDefaultZSampling( cs.zrg );
    }
    
    if ( inlfld_ )
    {
	inlfld_->setChecked( glinited ? pdd_->gridlines()->areInlinesShown() 
				     : true );
	const StepInterval<int> inlintv( cs.hrg.start.inl, cs.hrg.stop.inl,
					 cs.hrg.step.inl );
	inlspacingfld_->setValue(inlintv);
    }
    if ( crlfld_ )
    {
	crlfld_->setChecked( glinited ? pdd_->gridlines()->areCrosslinesShown() 
				     : true);
	const StepInterval<int> crlintv( cs.hrg.start.crl, cs.hrg.stop.crl,
					 cs.hrg.step.crl );
	crlspacingfld_->setValue(crlintv);
    }
    if ( zfld_ )
    {
	zfld_->setChecked( glinited ? pdd_->gridlines()->areZlinesShown() 
				    : true);
	StepInterval<int> zintv( (int)cs.zrg.start, (int)cs.zrg.stop,
				 (int)cs.zrg.step );
	zspacingfld_->setValue(zintv);
    }
}


#define mGetHrgSampling(dir)\
    StepInterval<int> dir##intv = dir##spacingfld_->getIStepInterval();\
    cs.hrg.start.dir = dir##intv.start;\
    cs.hrg.stop.dir = dir##intv.stop;\
    cs.hrg.step.dir = dir##intv.step;\


bool uiGridLinesDlg::acceptOK( CallBacker* )
{
    visBase::GridLines& gl = *pdd_->gridlines();
    CubeSampling cs;
    if ( inlfld_ ) { mGetHrgSampling(inl) };
    if ( crlfld_ ) { mGetHrgSampling(crl) };
    if ( zfld_ )
    {
	assign( cs.zrg, zspacingfld_->getFStepInterval() );
	cs.zrg.scale( 1/SI().zFactor() );
    }

    gl.setGridCubeSampling( cs );
    gl.setPlaneCubeSampling( pdd_->getCubeSampling() );
    if ( inlfld_ )
	gl.showInlines( inlfld_->isChecked() );
    if ( crlfld_ )
	gl.showCrosslines( crlfld_->isChecked() );
    if ( zfld_ )
	gl.showZlines( zfld_->isChecked() );

    gl.setLineStyle( lsfld_->getStyle() );

    return true;
}


