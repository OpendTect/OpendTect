/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui2dsip.h"

#include "uicoordsystem.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigeom2dsel.h"
#include "uigroup.h"
#include "uiimpexp2dgeom.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

#include "file.h"
#include "od_helpids.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "tabledef.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"


static const char* dlgtitle =
"Specify working area values.\n"
"No need to be precise, parts can lie outside the ranges.\n"
"The values will determine the size of the display box,\n"
"and provide some defaults e.g. for 3D horizon generation.";

ui2DSurvInfoProvider::ui2DSurvInfoProvider()
{}

ui2DSurvInfoProvider::~ui2DSurvInfoProvider()
{}


class ui2DDefSurvInfoDlg : public uiDialog
{ mODTextTranslationClass(ui2DDefSurvInfoDlg);
public:

ui2DDefSurvInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Survey setup for 2D only"),
				 toUiString(dlgtitle),
				 mODHelpKey(m2DDefSurvInfoDlgHelpID) ))
{
    FloatInpSpec fis;
    DoubleInpSpec dis;

    xrgfld_ = new uiGenInput( this, uiStrings::phrXcoordinate(
				       uiStrings::sRange().toLower()),
			      dis, dis );
    xrgfld_->setElemSzPol( uiObject::Medium );

    yrgfld_ = new uiGenInput( this, uiStrings::phrYcoordinate(
				       uiStrings::sRange().toLower()),
			      dis, dis );
    yrgfld_->attach( alignedBelow, xrgfld_ );
    yrgfld_->setElemSzPol( uiObject::Medium );

    grdspfld_ = new uiGenInput( this,
				tr("Default grid spacing for horizons"), fis );
    grdspfld_->attach( alignedBelow, yrgfld_ );

    ismfld_ = new uiGenInput( this, tr("Coordinates are in"),
	    BoolInpSpec(true,uiStrings::sMeter(),uiStrings::sFeet()) );
    ismfld_->attach( alignedBelow, grdspfld_ );

    uiSeparator* optsep = new uiSeparator( this, "Optional" );
    optsep->attach( stretchedBelow, ismfld_ );

    uiLabel* zrglbl = new uiLabel( this, tr("Optional:") );
    zrglbl->attach( leftBorder );
    zrglbl->attach( ensureBelow, optsep );

    const uiString zunitlbl( UnitOfMeasure::surveyDefZUnitAnnot(true,true) );
    zfld_ = new uiGenInput( this, tr( "Z range %1").arg( zunitlbl),
			DoubleInpIntervalSpec(true).setName("Z Start",0)
						   .setName("Z Stop",1)
						   .setName("Z step",2) );
    zfld_->attach( alignedBelow, ismfld_ );
    zfld_->attach( ensureBelow, zrglbl );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* ) override
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

    return true;
}

    uiGenInput*		grdspfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		ismfld_;
    uiGenInput*		zfld_;

};


uiDialog* ui2DSurvInfoProvider::dialog( uiParent* p )
{
    return new ui2DDefSurvInfoDlg( p );
}


bool uiSurvInfoProvider::getRanges( TrcKeyZSampling& cs, Coord crd[3],
					   Coord c0, Coord c1, double grdsp )
{
    const Coord d( c1.x - c0.x, c1.y - c0.y );
    const int nrinl = (int)(d.x / grdsp + 1.5);
    const int nrcrl = (int)(d.y / grdsp + 1.5);
    if ( nrinl < 2 && nrcrl < 2 )
	mErrRet(od_static_tr("getRanges",
			"Coordinate ranges are less than one trace distance"))

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
    return true;
}

#define cDefaultZMaxS 6.0f
#define cDefaultZMaxM 6000.0f
#define cDefaultZMaxF 10000.0f
#define cDefaultSrS 0.004f
#define cDefaultSrM 5.0f
#define cDefaultSrF 15.0f

bool ui2DSurvInfoProvider::getInfo( uiDialog* din, TrcKeyZSampling& cs,
				    Coord crd[3] )
{
    xyft_ = false;
    mDynamicCastGet(ui2DDefSurvInfoDlg*,dlg,din)
    if ( !dlg )
    {
	pErrMsg("Huh?");
	return false;
    }

    if ( dlg->uiResult() != 1 )
	return false; // cancelled

    Coord c0( dlg->xrgfld_->getDValue(0), dlg->yrgfld_->getDValue(0) );
    Coord c1( dlg->xrgfld_->getDValue(1), dlg->yrgfld_->getDValue(1) );
    if ( c0.x > c1.x ) Swap( c0.x, c1.x );
    if ( c0.y > c1.y ) Swap( c0.y, c1.y );
    const double grdsp = dlg->grdspfld_->getDValue();
    if ( !uiSurvInfoProvider::getRanges(cs,crd,c0,c1,grdsp) )
	return false;

    const StepInterval<float> zrg = dlg->zfld_->getFStepInterval();
    const bool hasstart = !mIsUdf(zrg.start);
    const bool hasstop = !mIsUdf(zrg.stop);
    const bool hasstep = !mIsUdf(zrg.step);
    const float start = hasstart ? zrg.start : 0.f;
    if ( SI().zIsTime() )
	cs.zsamp_.set( start/1000, hasstop ? zrg.stop/1000 : cDefaultZMaxS,
			hasstep ? zrg.step/1000 : cDefaultSrS );
    else if ( SI().zInFeet() )
	cs.zsamp_.set( start, hasstop ? zrg.stop : cDefaultZMaxF,
		hasstep ? zrg.step : cDefaultSrF );
    else
	cs.zsamp_.set( start, hasstop ? zrg.stop : cDefaultZMaxM,
		hasstep ? zrg.step : cDefaultSrM );

    xyft_ = !dlg->ismfld_->getBoolValue();

    return true;
}


void ui2DSurvInfoProvider::fillLogPars( IOPar& par ) const
{
    uiSurvInfoProvider::fillLogPars( par );
    par.set( sKey::CrFrom(), "Min/max X/Y coordinates" );
}



// uiNavSurvInfoProvider
class uiNavReadDlg : public uiImp2DGeom
{ mODTextTranslationClass(uiNavReadDlg)
public:
uiNavReadDlg( uiParent* p )
    : uiImp2DGeom(p)
{
    setModal( true );
    setOkCancelText( uiStrings::sContinue(), uiStrings::sCancel() );

    singlemultifld_->valuechanged.notify( mCB(this,uiNavReadDlg,singmultCB2) );
    linefld_->display( false );

    crssel_ = new Coords::uiCoordSystemSel( this );
    crssel_->attach( alignedBelow, dataselfld_ );
}


void singmultCB2( CallBacker* )
{
    linefld_->display( false );
}


bool fillGeom2D( ObjectSet<Survey::Geometry2D>& geoms )
{
    const bool singleline = singlemultifld_->getBoolValue();
    if ( singleline )
    {
	Survey::Geometry2D* geom2d =
		new Survey::Geometry2D( "New Geometry" );
	geom2d->ref();
	fillGeom( *geom2d );
	geoms += geom2d;
	return true;
    }

    return fillGeom( geoms );
}


bool acceptOK( CallBacker* ) override
{
    fname_ = fnmfld_->fileName();
    if ( !File::exists(fname_) || File::isEmpty(fname_) )
    { uiMSG().error(uiStrings::sInvInpFile()); return false; }

    const bool isll = geomfd_->bodyinfos_.last()->selection_.form_ == 1;
    if ( isll && !crssel_->getCoordSystem()->isProjection() )
    {
	uiMSG().error( tr("Please select a Coordinate System "
			  "when input file has Lat/Long."));
	return false;
    }

    return true;
}

    BufferString		fname_;
    Coords::uiCoordSystemSel*	crssel_;

};


uiNavSurvInfoProvider::uiNavSurvInfoProvider()
{
}


uiNavSurvInfoProvider::~uiNavSurvInfoProvider()
{
    deepUnRef( geoms_ );
}


const char* uiNavSurvInfoProvider::usrText() const
{ return "Scan Navigation Data"; }

const char* uiNavSurvInfoProvider::iconName() const
{ return "tree-geom2d"; }


uiDialog* uiNavSurvInfoProvider::dialog( uiParent* p )
{
    return new uiNavReadDlg( p );
}


bool uiNavSurvInfoProvider::getInfo( uiDialog* dlg, TrcKeyZSampling& tkzs,
				     Coord crd[3] )
{
    filename_.setEmpty();
    mDynamicCastGet(uiNavReadDlg*,navdlg,dlg)
    if ( !navdlg )
	return false;

    filename_ = navdlg->fname_;
    coordsystem_ = navdlg->crssel_->getCoordSystem();

    deepUnRef( geoms_ );
    if ( !navdlg->fillGeom2D(geoms_) )
	return false;

    Interval<double> xrg( mUdf(double), -mUdf(double) );
    Interval<double> yrg( mUdf(double), -mUdf(double) );
    for ( int idx=0; idx<geoms_.size(); idx++ )
    {
	Survey::Geometry2D* geom2d = geoms_[idx];
	const TypeSet<PosInfo::Line2DPos> l2dpos = geom2d->data().positions();
	for ( int idy=0; idy<l2dpos.size(); idy++ )
	{
	    xrg.include( l2dpos[idy].coord_.x, false );
	    yrg.include( l2dpos[idy].coord_.y, false );
	}
    }

    Coord c0( xrg.start, yrg.start );
    Coord c1( xrg.stop, yrg.stop );
    const double grdsp = 25.;
    return uiSurvInfoProvider::getRanges(tkzs,crd,c0,c1,grdsp);
}


void uiNavSurvInfoProvider::fillLogPars( IOPar& par ) const
{
    uiSurvInfoProvider::fillLogPars( par );
    if ( !filename_.isEmpty() )
	par.set( sKey::CrFrom(), filename_ );
}


IOPar* uiNavSurvInfoProvider::getImportPars() const
{
    return new IOPar;
}


void uiNavSurvInfoProvider::startImport( uiParent*, const IOPar& )
{
    uiStringSet errors;
    for ( int idx=0; idx<geoms_.size(); idx++ )
    {
	geoms_[idx]->dataAdmin().setZRange( SI().zRange(false) );
	uiString errmsg;
	if ( !Survey::GMAdmin().write(*geoms_[idx],errmsg) )
	    errors.add( errmsg );
    }

    if ( !errors.isEmpty() )
    {
	uiMSG().errorWithDetails( errors, tr("Error during import:") );
	return;
    }

    uiMSG().message( tr("Geometry successfully imported.") );
}


const char* uiNavSurvInfoProvider::importAskQuestion() const
{ return "Proceed to import files used to setup survey?"; }


IOPar* uiNavSurvInfoProvider::getCoordSystemPars() const
{
    if ( !coordsystem_ )
	return 0;

    IOPar* crspar = new IOPar;
    coordsystem_->fillPar( *crspar );
    return crspar;
}
