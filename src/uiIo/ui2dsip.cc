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
#include "uimsg.h"
#include "uigeninput.h"
#include "cubesampling.h"
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
    grdspfld = new uiGenInput( this, "Default grid spacing for horizons",
	    			 FloatInpSpec() );
    DoubleInpSpec dis;
    xrgfld = new uiGenInput( this, "X-coordinate range", dis, dis );
    xrgfld->attach( alignedBelow, grdspfld );
    yrgfld = new uiGenInput( this, "Y-coordinate range", dis, dis );
    yrgfld->attach( alignedBelow, xrgfld );
    ismfld = new uiGenInput( this, "Above values are in",
	    			    BoolInpSpec(true,"Meters","Feet") );
    ismfld->attach( alignedBelow, yrgfld );
}

    uiGenInput*		grdspfld;
    uiGenInput*		xrgfld;
    uiGenInput*		yrgfld;
    uiGenInput*		ismfld;

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

    double grdsp = dlg->grdspfld->getdValue();
    Coord c0( dlg->xrgfld->getdValue(0), dlg->yrgfld->getdValue(0) );
    Coord c1( dlg->xrgfld->getdValue(1), dlg->yrgfld->getdValue(1) );
    if ( grdsp < 0 ) grdsp = -grdsp;
    if ( c0.x > c1.x ) Swap( c0.x, c1.x );
    if ( c0.y > c1.y ) Swap( c0.y, c1.y );
    const Coord d( c1.x - c0.x, c1.y - c0.y );
    if ( grdsp < 0.1 )
	mErrRet("Grid spacing should be > 0.1")
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

    cs.zrg.start = cs.zrg.stop = mSetUdf(cs.zrg.step);
    xyft_ = !dlg->ismfld->getBoolValue();
    return true;
}
