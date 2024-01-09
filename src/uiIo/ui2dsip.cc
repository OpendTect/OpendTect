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
#include "uigeninputdlg.h"
#include "uiimpexp2dgeom.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

#include "dirlist.h"
#include "filepath.h"
#include "hiddenparam.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "posinfo2d.h"
#include "sharedlibs.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "tabledef.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"


static HiddenParam<ui2DSurvInfoProvider,IOPar*> hp_2dsip(nullptr);


static const char* dlgtitle =
"Specify working area values.\n"
"No need to be precise, parts can lie outside the ranges.\n"
"The values will determine the size of the display box,\n"
"and provide some defaults e.g. for 3D horizon generation.";

ui2DSurvInfoProvider::ui2DSurvInfoProvider()
{
    hp_2dsip.setParam( this, nullptr );
}

ui2DSurvInfoProvider::~ui2DSurvInfoProvider()
{
    hp_2dsip.removeAndDeleteParam( this );
}


class ui2DDefSurvInfoDlg : public uiDialog
{ mODTextTranslationClass(ui2DDefSurvInfoDlg);
public:

ui2DDefSurvInfoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Survey Setup: Enter X/Y Ranges"),
				 toUiString(dlgtitle),
				 mODHelpKey(m2DDefSurvInfoDlgHelpID) ))
{
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
				tr("Default grid spacing for horizons"),
				FloatInpSpec() );
    grdspfld_->attach( alignedBelow, yrgfld_ );

    crsfld_ = new Coords::uiCoordSystemSel( this, true, false, nullptr );
    crsfld_->attach( alignedBelow, grdspfld_ );

    uiSeparator* optsep = new uiSeparator( this, "Optional" );
    optsep->attach( stretchedBelow, crsfld_ );

    uiLabel* zrglbl = new uiLabel( this, tr("Optional:") );
    zrglbl->attach( leftBorder );
    zrglbl->attach( ensureBelow, optsep );

    const uiString zunitlbl( UnitOfMeasure::surveyDefZUnitAnnot(true,true) );
    zfld_ = new uiGenInput( this, tr( "Z range %1").arg( zunitlbl),
			DoubleInpIntervalSpec(true).setName("Z Start",0)
						   .setName("Z Stop",1)
						   .setName("Z step",2) );
    zfld_->attach( alignedBelow, crsfld_ );
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

    if ( !crsfld_->getCoordSystem() )
	mErrRet( tr("Please select a Coordinate System") );

    return true;
}

    uiGenInput*			grdspfld_;
    uiGenInput*			xrgfld_;
    uiGenInput*			yrgfld_;
    Coords::uiCoordSystemSel*	crsfld_;
    uiGenInput*			zfld_;

};


uiDialog* ui2DSurvInfoProvider::dialog( uiParent* p )
{
    return new ui2DDefSurvInfoDlg( p );
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

    auto crs = dlg->crsfld_->getCoordSystem();
    if ( crs )
    {
	xyft_ = crs->isFeet();
	auto* crspar = new IOPar;
	crs->fillPar( *crspar );
	hp_2dsip.setParam( this, crspar );
    }

    return true;
}


void ui2DSurvInfoProvider::fillLogPars( IOPar& par ) const
{
    uiSurvInfoProvider::fillLogPars( par );
    par.set( sKey::CrFrom(), "Min/max X/Y coordinates" );
}


IOPar* ui2DSurvInfoProvider::getCoordSystemPars() const
{
    auto* crspar = hp_2dsip.getParam( this );
    return crspar ? new IOPar(*crspar) : nullptr;
}



// uiSurvInfoProvider
uiSurvInfoProvider::uiSurvInfoProvider()
{}


uiSurvInfoProvider::~uiSurvInfoProvider()
{}


static void ensureMinSize( int minnum, int& nrnodes, double& originxy,
			   double grdsp )
{
    if ( minnum%2 == 0 )
       minnum++; // Use odd number for better centering

    if ( nrnodes < minnum )
    {
	const int diff = minnum - nrnodes;
	nrnodes = minnum;
	originxy -= (diff/2) * grdsp;
    }
}


#define mMinSize 21
bool uiSurvInfoProvider::getRanges( TrcKeyZSampling& cs, Coord crd[3],
					   Coord c0, Coord c1, double grdsp )
{
    const Coord d( c1.x - c0.x, c1.y - c0.y );
    int nrinl = (int)(d.x / grdsp + 1.5);
    int nrcrl = (int)(d.y / grdsp + 1.5);
    if ( nrinl < 2 && nrcrl < 2 )
	mErrRet(od_static_tr("getRanges",
			"Coordinate ranges are less than one trace distance"))

    ensureMinSize( mMinSize, nrinl, c0.x, grdsp );
    ensureMinSize( mMinSize, nrcrl, c0.y, grdsp );
    if ( nrinl < nrcrl )
	ensureMinSize( nrcrl/10, nrinl, c0.x, grdsp );
    else
	ensureMinSize( nrinl/10, nrcrl, c0.y, grdsp );

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


void uiSurvInfoProvider::addPluginsInfoProviders()
{
    const FilePath sipdatafp( mGetSWDirDataDir(), "SurveyProviders" );
    if ( !sipdatafp.exists() )
	return;

    using VoidVoidFn = void(*)(void);
    const DirList dl( sipdatafp.fullPath(), File::FilesInDir, "*.txt" );
    BufferString libname;
    libname.setBufSize( 256 );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const FilePath dirname( dl.get( idx ).buf() );
	const BufferString piname = dirname.baseName();
	if ( piname.isEmpty() )
	    continue;

	libname.setEmpty();
	SharedLibAccess::getLibName( piname.buf(), libname.getCStr(),
				     libname.bufSize() );
	const FilePath fp( GetLibPlfDir(), libname );
	if ( !fp.exists() )
	    continue;

	const SharedLibAccess pisha( fp.fullPath() );
	if ( !pisha.isOK() )
	    return;

	const BufferString sipfuncnm( piname.str(), "InitSIP" );
	VoidVoidFn sipinitfn = (VoidVoidFn)pisha.getFunction( sipfuncnm.str() );
	if ( sipinitfn )
	    (*sipinitfn)();
	//Do NOT close the handle, as the plugin must remain loaded
    }
}



// uiNavSurvInfoProvider
class uiNavReadDlg : public uiImp2DGeom
{ mODTextTranslationClass(uiNavReadDlg)
public:
uiNavReadDlg( uiParent* p )
    : uiImp2DGeom(p,"",true)
{
    setModal( true );
    setOkCancelText( uiStrings::sContinue(), uiStrings::sCancel() );

    crsfld_ = new Coords::uiCoordSystemSel( this );
    crsfld_->attach( alignedBelow, dataselfld_ );

    uiSeparator* optsep = new uiSeparator( this, "Optional" );
    optsep->attach( stretchedBelow, crsfld_ );

    uiLabel* zrglbl = new uiLabel( this, tr("Optional:") );
    zrglbl->attach( leftBorder );
    zrglbl->attach( ensureBelow, optsep );

    const uiString zunitlbl( UnitOfMeasure::surveyDefZUnitAnnot(true,true) );
    zfld_ = new uiGenInput( this, tr( "Z range %1").arg( zunitlbl),
			DoubleInpIntervalSpec(true).setName("Z Start",0)
						   .setName("Z Stop",1)
						   .setName("Z step",2) );
    zfld_->attach( alignedBelow, crsfld_ );
    zfld_->attach( ensureBelow, zrglbl );

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
    if ( isll && !crsfld_->getCoordSystem()->isProjection() )
    {
	uiMSG().error( tr("Please select a Coordinate System "
			  "when input file has Lat/Long."));
	return false;
    }

    return true;
}

    BufferString		fname_;
    Coords::uiCoordSystemSel*	crsfld_;
    uiGenInput*			zfld_;

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
    coordsystem_ = navdlg->crsfld_->getCoordSystem();

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
    if ( !uiSurvInfoProvider::getRanges(tkzs,crd,c0,c1,grdsp) )
	return false;

    const StepInterval<float> zrg = navdlg->zfld_->getFStepInterval();
    const bool hasstart = !mIsUdf(zrg.start);
    const bool hasstop = !mIsUdf(zrg.stop);
    const bool hasstep = !mIsUdf(zrg.step);
    const float start = hasstart ? zrg.start : 0.f;
    if ( SI().zIsTime() )
	tkzs.zsamp_.set( start/1000, hasstop ? zrg.stop/1000 : cDefaultZMaxS,
			hasstep ? zrg.step/1000 : cDefaultSrS );
    else if ( SI().zInFeet() )
	tkzs.zsamp_.set( start, hasstop ? zrg.stop : cDefaultZMaxF,
		hasstep ? zrg.step : cDefaultSrF );
    else
	tkzs.zsamp_.set( start, hasstop ? zrg.stop : cDefaultZMaxM,
		hasstep ? zrg.step : cDefaultSrM );

    return true;
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


void uiNavSurvInfoProvider::startImport( uiParent* p, const IOPar& )
{
    uiStringSet errors;
    if ( geoms_.size() == 1 )
    {
	uiGenInputDlg dlg( p, tr("Specify Line Name"),
			   tr("Enter the name of the 2D Line") );
	if ( !dlg.go() )
	    return;

	const BufferString linename = dlg.text();
	if ( !linename.isEmpty() )
	    geoms_[0]->dataAdmin().setLineName( linename );
    }

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
	return nullptr;

    IOPar* crspar = new IOPar;
    coordsystem_->fillPar( *crspar );
    return crspar;
}
