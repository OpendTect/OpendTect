/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uigridlinesdlg.cc,v 1.2 2006-02-09 13:55:53 cvshelene Exp $
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
    name##spacingfld_ = new uiGenInput( this, "spacing",\
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
    

#define mSetDefaultHrgStep(dir)\
	interval = cs.hrg.stop.dir - cs.hrg.start.dir;\
	if ( interval/10 < 50 )\
	    cs.hrg.step.dir = 50;\
	else if ( interval/10 < 100 )\
	    cs.hrg.step.dir = 100;\
	else\
	    cs.hrg.step.dir = 200;\
	\
	cs.hrg.start.dir = cs.hrg.step.dir * \
		    ( ceil( (float)cs.hrg.start.dir/(float)cs.hrg.step.dir ) );\
	cs.hrg.stop.dir = cs.hrg.step.dir * \
		    ( floor( (float)cs.hrg.stop.dir/(float)cs.hrg.step.dir ) );\


#define mSetDefaultZrgStep()\
	interval = ( cs.zrg.stop - cs.zrg.start ) * SI().zFactor();\
	cs.zrg.step = interval<1000 ? 100 : 1000;\
	cs.zrg.start = cs.zrg.step * \
	    ceil( (float)cs.zrg.start * SI().zFactor() / (float)cs.zrg.step );\
	cs.zrg.stop = cs.zrg.step * \
	    floor( (float)cs.zrg.stop * SI().zFactor() / (float)cs.zrg.step );\


void uiGridLinesDlg::setParameters()
{
    bool glinited = !pdd_->gridlines()->getGridCubeSampling().isEmpty();
    CubeSampling cs;
    if ( glinited )
    {
	cs = pdd_->gridlines()->getGridCubeSampling();
	cs.zrg.scale( SI().zFactor() );
    }
    else
    {
	float interval;
	mSetDefaultHrgStep(inl);
	mSetDefaultHrgStep(crl);
	mSetDefaultZrgStep();
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


#define mGetZrgSampling()\
    StepInterval<float> zintv = zspacingfld_->getFStepInterval();\
    cs.zrg.start = zintv.start/SI().zFactor();\
    cs.zrg.stop = zintv.stop/SI().zFactor();\
    cs.zrg.step = zintv.step/SI().zFactor();\


bool uiGridLinesDlg::acceptOK( CallBacker* )
{
    visBase::GridLines& gl = *pdd_->gridlines();
    CubeSampling cs;
    if ( inlfld_ ) { mGetHrgSampling(inl) };
    if ( crlfld_ ) { mGetHrgSampling(crl) };
    if ( zfld_ ) { 
//	mGetZrgSampling() };
	StepInterval<float> zintv = zspacingfld_->getFStepInterval();
	cs.zrg.start = zintv.start/SI().zFactor();
	cs.zrg.stop = zintv.stop/SI().zFactor();
	cs.zrg.step = zintv.step/SI().zFactor();
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


