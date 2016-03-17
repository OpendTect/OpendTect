/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2006
________________________________________________________________________

-*/

#include "uigridlinesdlg.h"

#include "draw.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uisellinest.h"
#include "visgridlines.h"
#include "visplanedatadisplay.h"
#include "od_helpids.h"

#include <math.h>


#define mCreateGridFld( name, type ) \
    uiString lbl = label; lbl.arg( type ); \
    name##fld_ = new uiCheckBox( this, lbl ); \
    name##fld_->activated.notify( mCB(this,uiGridLinesDlg,showGridLineCB) ); \
    name##spacingfld_ = new uiGenInput( this, spacingstr, \
					IntInpIntervalSpec(true) ); \
    name##spacingfld_->attach( leftAlignedBelow, name##fld_ );


uiGridLinesDlg::uiGridLinesDlg( uiParent* p, visSurvey::PlaneDataDisplay* pdd )
    : uiDialog(p,uiDialog::Setup(tr("Set Grid Lines"),mNoDlgTitle,
				 mODHelpKey(mGridLinesDlgHelpID) ))
    , pdd_( pdd )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , zfld_( 0 )
    , inlspacingfld_( 0 )
    , crlspacingfld_( 0 )
    , zspacingfld_( 0 )
{
    const uiString spacingstr = tr("Spacing (Start/Stop)");

    uiString label = tr("Show %1 Grid");
    TrcKeyZSampling cs( pdd->getTrcKeyZSampling(true,true) );
    if ( cs.nrInl()>1 )
    { mCreateGridFld( inl, uiStrings::sInline() ) }
    if ( cs.nrCrl()>1 )
    { mCreateGridFld( crl, uiStrings::sCrossline() ) }
    if ( cs.nrZ()>1 )
    { mCreateGridFld( z, toUiString("Z") ); }

    if ( inlfld_ && crlfld_ )
	crlfld_->attach( leftAlignedBelow, inlspacingfld_ );

    OD::LineStyle lst;
    pdd->gridlines()->getLineStyle( lst );

    lsfld_ = new uiSelLineStyle( this, lst, tr("Line style") );
    if ( zfld_ )
    {
	zfld_->attach( leftAlignedBelow, inlfld_ ? inlspacingfld_
						 : crlspacingfld_ );
	lsfld_->attach( alignedBelow, zspacingfld_ );
    }
    else
	lsfld_->attach( alignedBelow, crlspacingfld_ );

    uiString allmsg = tr("Apply to all loaded %1");
    if ( OD::InlineSlice == pdd_->getOrientation() )
	allmsg.arg(uiStrings::sInline(mPlural));
    else if ( OD::CrosslineSlice == pdd_->getOrientation() )
	allmsg.arg( uiStrings::sCrossline(mPlural));
    else
	allmsg.arg(uiStrings::sZSlice(mPlural));
    applyallfld_ = new uiCheckBox( this, allmsg );
    applyallfld_->setChecked( true );
    applyallfld_->attach( alignedBelow, lsfld_ );

    setParameters();
}


void uiGridLinesDlg::showGridLineCB( CallBacker* cb )
{
    if ( inlspacingfld_ )
	inlspacingfld_->setSensitive( inlfld_->isChecked() );

    if ( crlspacingfld_ )
	crlspacingfld_->setSensitive( crlfld_->isChecked() );

    if ( zspacingfld_ )
	zspacingfld_->setSensitive( zfld_->isChecked() );

    lsfld_->setSensitive( (inlfld_ && inlfld_->isChecked()) ||
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


static void getDefaultTrcKeySampling( int& start, int& stop, int& step )
{
    const float width = mCast( float, stop - start );
    step = mNINT32( getDefaultStep(width) );

    start = step * (int)( Math::Ceil( (float)start/(float)step ) );
    stop = step * (int)( Math::Floor( (float)stop/(float)step ) );
}


static void getDefaultZSampling( StepInterval<float>& zrg )
{
    const float width = (zrg.stop-zrg.start) * SI().zDomain().userFactor();
    zrg.step = getDefaultStep( width );
    zrg.start = zrg.step *
	Math::Ceil( (float)zrg.start * SI().zDomain().userFactor()
		    / (float)zrg.step );
    zrg.stop = zrg.step *
	Math::Floor( (float)zrg.stop * SI().zDomain().userFactor()
		    / (float)zrg.step );
}


void uiGridLinesDlg::setParameters()
{
    const bool hasgl = !pdd_->gridlines()->getGridTrcKeyZSampling().isEmpty();
    TrcKeyZSampling cs = pdd_->getTrcKeyZSampling( true, true );
    if ( hasgl )
    {
	cs = pdd_->gridlines()->getGridTrcKeyZSampling();
	cs.zsamp_.scale( mCast(float,SI().zDomain().userFactor()) );
    }
    else
    {
	getDefaultTrcKeySampling( cs.hsamp_.start_.inl(), cs.hsamp_.stop_.inl(),
			       cs.hsamp_.step_.inl() );
	getDefaultTrcKeySampling( cs.hsamp_.start_.crl(), cs.hsamp_.stop_.crl(),
			       cs.hsamp_.step_.crl() );
	getDefaultZSampling( cs.zsamp_ );
    }

    if ( inlfld_ )
    {
	inlfld_->setChecked( pdd_->gridlines()->areInlinesShown() );
	inlspacingfld_->setValue( cs.hsamp_.inlRange() );
    }

    if ( crlfld_ )
    {
	crlfld_->setChecked( pdd_->gridlines()->areCrosslinesShown() );
	crlspacingfld_->setValue( cs.hsamp_.crlRange() );
    }

    if ( zfld_ )
    {
	zfld_->setChecked( pdd_->gridlines()->areZlinesShown() );
	zspacingfld_->setValue(
		StepInterval<int>(mNINT32(cs.zsamp_.start),
				  mNINT32(cs.zsamp_.stop),
				  mNINT32(cs.zsamp_.step)) );
    }

    showGridLineCB(0);
}


#define mGetHrgSampling(dir)\
    StepInterval<int> dir##intv = dir##spacingfld_->getIStepInterval();\
    cs.hsamp_.start_.dir() = dir##intv.start;\
    cs.hsamp_.stop_.dir() = dir##intv.stop;\
    cs.hsamp_.step_.dir() = dir##intv.step;\


bool uiGridLinesDlg::acceptOK( CallBacker* )
{
    TrcKeyZSampling cs;
    if ( inlfld_ ) { mGetHrgSampling(inl) };
    if ( crlfld_ ) { mGetHrgSampling(crl) };
    if ( zfld_ )
    {
	cs.zsamp_.setFrom( zspacingfld_->getFStepInterval() );
	cs.zsamp_.scale( 1.f/SI().zDomain().userFactor() );
    }

    if ( (inlfld_ && inlfld_->isChecked() && cs.hsamp_.step_.inl()==0) ||
	 (crlfld_ && crlfld_->isChecked() && cs.hsamp_.step_.crl()==0) ||
	 (zfld_ && zfld_->isChecked() && mIsZero(cs.zsamp_.step,mDefEps)) )
    {
	uiMSG().error( tr("Please make sure all steps are non-zero") );
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
	gl.setPlaneTrcKeyZSampling( pdd->getTrcKeyZSampling(true,true) );
	gl.setGridTrcKeyZSampling( cs );
	gl.showInlines( inlfld_ ? inlfld_->isChecked() : false );
	gl.showCrosslines( crlfld_ ? crlfld_->isChecked(): false );
	gl.showZlines( zfld_ ? zfld_->isChecked(): false );
	gl.setLineStyle( lsfld_->getStyle() );
    }

    return true;
}
