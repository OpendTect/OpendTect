/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2004
________________________________________________________________________

-*/

#include "ui2dsip.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "trckeyzsampling.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "od_helpids.h"


class ui2DDefSurvInfoDlg : public uiDialog
{ mODTextTranslationClass(ui2DDefSurvInfoDlg);
public:

ui2DDefSurvInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Survey setup for 2D only"), sDlgTitle(),
                                 mODHelpKey(m2DDefSurvInfoDlgHelpID) ))
{
    FloatInpSpec fis;
    DoubleInpSpec dis;

    uiGroup* maingrp = new uiGroup( this, "Main parameters" );
    grdspfld_ = new uiGenInput( maingrp, tr("Default grid spacing for horizons")
				,fis );
    xrgfld_ = new uiGenInput( maingrp, uiStrings::phrXcoordinate(
				       uiStrings::sRange()), dis, dis );
    xrgfld_->attach( alignedBelow, grdspfld_ );
    yrgfld_ = new uiGenInput( maingrp, uiStrings::phrYcoordinate(
				       uiStrings::sRange()), dis, dis );
    yrgfld_->attach( alignedBelow, xrgfld_ );
    ismfld_ = new uiGenInput( maingrp, tr("Above values are in"),
				      BoolInpSpec(true,uiStrings::sMeter(),
				      uiStrings::sFeet()) );
    ismfld_->attach( alignedBelow, yrgfld_ );

    uiSeparator* optsep = new uiSeparator( this, "Optional" );
    optsep->attach( stretchedBelow, maingrp );

    uiLabel* zrglbl = new uiLabel( this, tr("Optional:") );
    zrglbl->attach( leftBorder );
    zrglbl->attach( ensureBelow, optsep );

    uiGroup* optgrp = new uiGroup( this, "Optional parameters" );

    const uiString zunitlbl(UnitOfMeasure::surveyDefZUnitAnnot(true,true));
    zmaxfld_ = new uiGenInput( optgrp,
	       tr( "[Z-max %1]").arg( zunitlbl), fis );
    srfld_ = new uiGenInput( optgrp,
	     tr( "[Default sampling rate %1]").arg(zunitlbl), fis);
    srfld_->attach( alignedBelow, zmaxfld_ );

    optgrp->attach( alignedBelow, maingrp );
    optgrp->attach( ensureBelow, optsep );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define cDefaultTWTMax 6000.0f
#define cDefaultZMaxm 6000.0f
#define cDefaultZMaxft 10000.0f
#define cDefautSRms 2.0f
#define cDefautSRm 5.0f
#define cDefautSRft 15.0f


bool acceptOK( CallBacker* )
{
    const float grdsp = grdspfld_->getFValue();
    if ( mIsUdf(grdsp) )
	mErrRet( tr("Invalid grid spacing") )
    else if ( grdsp < 0 )
	mErrRet( tr("Grid spacing should be strictly positive") )
    else if ( grdsp < 0.1 )
	mErrRet( tr("Grid spacing should be > 0.1") )

    const Coord c0( xrgfld_->getDValue(0), yrgfld_->getDValue(0) );
    const Coord c1( xrgfld_->getDValue(1), yrgfld_->getDValue(1) );
    if ( mIsUdf(c0) || mIsUdf(c1) )
	mErrRet(tr("Invalid input coordinates"))

    const bool zintime = SI().zDomain().isTime();
    const bool zinft = SI().depthsInFeet();
    const float defzmax = zintime ? cDefaultTWTMax
				  : ( zinft ? cDefaultZMaxft : cDefaultZMaxm );
    if ( mIsUdf(zmaxfld_->getFValue()) )
	zmaxfld_->setValue( defzmax );

    const float defsr = zintime ? cDefautSRms
				: ( zinft ? cDefautSRft : cDefautSRm );
    if ( mIsUdf(srfld_->getFValue()) )
	srfld_->setValue( defsr );

    if ( zmaxfld_->getFValue() < 0 )
	mErrRet(tr("Z Max should be strictly positive"))

	if (srfld_->getFValue() < 0)
	mErrRet(tr("The default sampling rate should be strictly positive"))

	    if (zmaxfld_->getFValue() < srfld_->getFValue())
	mErrRet(tr("Z Max should be larger than the sampling rate"))

    return true;
}

    uiGenInput*		grdspfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		ismfld_;
    uiGenInput*		zmaxfld_;
    uiGenInput*		srfld_;
    static const uiString sDlgTitle() {return tr("Specify working area values."
		  "\nNo need to be precise, parts can lie outside the ranges.\n"
		  "The values will determine the size of the display box,\n"
		  "and provide some defaults a.o. for 3D horizon generation.");}
};


uiDialog* ui2DSurvInfoProvider::dialog( uiParent* p )
{
    return new ui2DDefSurvInfoDlg( p );
}


bool ui2DSurvInfoProvider::getInfo( uiDialog* din, TrcKeyZSampling& cs,
				      Coord crd[3] )
{
    xyft_ = false;
    if ( !din ) return false;
    mDynamicCastGet(ui2DDefSurvInfoDlg*,dlg,din)
    if ( !dlg ) { pErrMsg("Huh?"); return false; }
    else if ( dlg->uiResult() != 1 ) return false; // cancelled

    Coord c0( dlg->xrgfld_->getDValue(0), dlg->yrgfld_->getDValue(0) );
    Coord c1( dlg->xrgfld_->getDValue(1), dlg->yrgfld_->getDValue(1) );
    if ( c0.x > c1.x ) Swap( c0.x, c1.x );
    if ( c0.y > c1.y ) Swap( c0.y, c1.y );
    const Coord d( c1.x - c0.x, c1.y - c0.y );
    const double grdsp = dlg->grdspfld_->getDValue();
    const int nrinl = (int)(d.x / grdsp + 1.5);
    const int nrcrl = (int)(d.y / grdsp + 1.5);
    if ( nrinl < 2 && nrcrl < 2 )
	mErrRet(tr("Coordinate ranges are less than one trace distance"))

    cs.hsamp_.start_.inl() = cs.hsamp_.start_.crl() = 10000;
    cs.hsamp_.step_.inl() = cs.hsamp_.step_.crl() = 1;
    cs.hsamp_.stop_.inl() = 10000 + nrinl - 1;
    cs.hsamp_.stop_.crl() = 10000 + nrcrl -1;

    Coord cmax( c0.x + grdsp*(nrinl-1), c0.y + grdsp*(nrcrl-1) );
    if ( cmax.x < c0.x ) Swap( cmax.x, c0.x );
    if ( cmax.y < c0.y ) Swap( cmax.y, c0.y );
    crd[0] = c0;
    crd[1] = cmax;
    crd[2] = Coord( c0.x, cmax.y );

    const float zfac = SI().showZ2UserFactor();
    cs.zsamp_.start = 0.f;
    cs.zsamp_.stop = dlg->zmaxfld_->getFValue() / zfac;
    cs.zsamp_.step = dlg->srfld_->getFValue() / zfac;

    xyft_ = !dlg->ismfld_->getBoolValue();

    return true;
}
