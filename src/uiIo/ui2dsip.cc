/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ui2dsip.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "errh.h"

static const char* dlgtitle =
"Specify working area values.\n"
"No need to be precise, parts can lie outside the ranges.\n"
"The values will determine the size of the display box,\n"
"and provide some defaults a.o. for 3D horizon generation.";


class ui2DDefSurvInfoDlg : public uiDialog
{
public:

ui2DDefSurvInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey setup for 2D only",
				 dlgtitle,"0.3.8"))
{
    FloatInpSpec fis;
    DoubleInpSpec dis;

    uiGroup* maingrp = new uiGroup( this, "Main parameters" );
    grdspfld_ = new uiGenInput( maingrp, "Default grid spacing for horizons",
	    			fis );
    xrgfld_ = new uiGenInput( maingrp, "X-coordinate range", dis, dis );
    xrgfld_->attach( alignedBelow, grdspfld_ );
    yrgfld_ = new uiGenInput( maingrp, "Y-coordinate range", dis, dis );
    yrgfld_->attach( alignedBelow, xrgfld_ );
    ismfld_ = new uiGenInput( maingrp, "Above values are in",
	    			      BoolInpSpec(true,"Meters","Feet") );
    ismfld_->attach( alignedBelow, yrgfld_ );

    uiSeparator* optsep = new uiSeparator( this, "Optional" );
    optsep->attach( stretchedBelow, maingrp );

    uiLabel* zrglbl = new uiLabel( this, "Optional:" );
    zrglbl->attach( leftBorder );
    zrglbl->attach( ensureBelow, optsep );

    uiGroup* optgrp = new uiGroup( this, "Optional parameters" );

    const BufferString zunitlbl(UnitOfMeasure::surveyDefZUnitAnnot(true,true));
    zmaxfld_ = new uiGenInput( optgrp,
	   		       BufferString( "[Z-max ", zunitlbl, "]" ), fis );
    srfld_ = new uiGenInput( optgrp,
		 BufferString( "[Default sampling rate ", zunitlbl, "]" ), fis);
    srfld_->attach( alignedBelow, zmaxfld_ );

    optgrp->attach( alignedBelow, maingrp );
    optgrp->attach( ensureBelow, optsep );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define cDefaultTWTMax 6000
#define cDefaultZMaxm 6000
#define cDefaultZMaxft 10000
#define cDefautSRms 2
#define cDefautSRm 5
#define cDefautSRft 15


bool acceptOK( CallBacker* )
{
    const float grdsp = grdspfld_->getfValue();
    if ( mIsUdf(grdsp) )
	mErrRet( "Invalid grid spacing" )
    if ( grdsp < 0 )
	mErrRet( "Grid spacing should be strictly positive" )
    if ( grdsp < 0.1 )
	mErrRet("Grid spacing should be > 0.1")

    const Coord c0( xrgfld_->getdValue(0), yrgfld_->getdValue(0) );
    const Coord c1( xrgfld_->getdValue(1), yrgfld_->getdValue(1) );
    if ( mIsUdf(c0) || mIsUdf(c1) )
	mErrRet( "Invalid input coordinates" )

    const bool zintime = SI().zDomain().isTime();
    const bool zinft = SI().depthsInFeet();
    const float defzmax = zintime ? cDefaultTWTMax
				  : ( zinft ? cDefaultZMaxft : cDefaultZMaxm );
    if ( mIsUdf(zmaxfld_->getfValue()) )
	zmaxfld_->setValue( defzmax );

    const float defsr = zintime ? cDefautSRms
				: ( zinft ? cDefautSRft : cDefautSRm );
    if ( mIsUdf(srfld_->getfValue()) )
	srfld_->setValue( defsr );

    if ( zmaxfld_->getfValue() < 0 )
	mErrRet( "Z Max should be strictly positive" )

    if ( srfld_->getfValue() < 0 )
	mErrRet( "The default sampling rate should be strictly positive" )

    if ( zmaxfld_->getfValue() < srfld_->getfValue() )
	mErrRet( "Z Max should be larger than the sampling rate" )

    return true;
}

    uiGenInput*		grdspfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		ismfld_;
    uiGenInput*		zmaxfld_;
    uiGenInput*		srfld_;

};


uiDialog* ui2DSurvInfoProvider::dialog( uiParent* p )
{
    return new ui2DDefSurvInfoDlg( p );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool ui2DSurvInfoProvider::getInfo( uiDialog* din, CubeSampling& cs,
				      Coord crd[3] )
{
    xyft_ = false;
    if ( !din ) return false;
    mDynamicCastGet(ui2DDefSurvInfoDlg*,dlg,din)
    if ( !dlg ) { pErrMsg("Huh?"); return false; }

    Coord c0( dlg->xrgfld_->getdValue(0), dlg->yrgfld_->getdValue(0) );
    Coord c1( dlg->xrgfld_->getdValue(1), dlg->yrgfld_->getdValue(1) );
    if ( c0.x > c1.x ) Swap( c0.x, c1.x );
    if ( c0.y > c1.y ) Swap( c0.y, c1.y );
    const Coord d( c1.x - c0.x, c1.y - c0.y );
    const double grdsp = dlg->grdspfld_->getdValue();
    const int nrinl = (int)(d.x / grdsp + 1.5);
    const int nrcrl = (int)(d.y / grdsp + 1.5);
    if ( nrinl < 2 && nrcrl < 2 )
	mErrRet("Coordinate ranges are less than one trace distance")

    cs.hrg.start.inl = cs.hrg.start.crl = 10000;
    cs.hrg.step.inl = cs.hrg.step.crl = 1;
    cs.hrg.stop.inl = 10000 + nrinl - 1; cs.hrg.stop.crl = 10000 + nrcrl - 1;

    Coord cmax( c0.x + grdsp*(nrinl-1), c0.y + grdsp*(nrcrl-1) );
    if ( cmax.x < c0.x ) Swap( cmax.x, c0.x );
    if ( cmax.y < c0.y ) Swap( cmax.y, c0.y );
    crd[0] = c0;
    crd[1] = cmax;
    crd[2] = Coord( c0.x, cmax.y );

    const float zfac = SI().showZ2UserFactor();
    cs.zrg.start = 0.f;
    cs.zrg.stop = dlg->zmaxfld_->getfValue() / zfac;
    cs.zrg.step = dlg->srfld_->getfValue() / zfac;

    xyft_ = !dlg->ismfld_->getBoolValue();

    return true;
}
