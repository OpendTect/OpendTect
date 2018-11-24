/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2004
________________________________________________________________________

-*/

#include "uisipimpl.h"

#include "uidialog.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisurveyselect.h"
#include "uisurvinfoed.h"

#include "coordsystem.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"


uiSurvInfoProvider* uiSurvInfoProvider::getByName( const char* nm )
{
    const ObjectSet<uiSurvInfoProvider>& sips
		= uiSurveyInfoEditor::survInfoProvs();
    const FixedString sipnm( nm );
    for ( int idx=0; idx<sips.size(); idx++ )
	if ( sipnm == toString(sips[idx]->usrText()) )
	    return const_cast<uiSurvInfoProvider*>( sips[idx] );
    return 0;
}


uiSurvInfoProvider* uiSurvInfoProvider::getByName( const uiString& nm )
{
    return getByName( toString(nm) );
}


class ui2DDefSurvInfoDlg : public uiDialog
{ mODTextTranslationClass(ui2DDefSurvInfoDlg)
public:

    typedef uiSurvInfoProvider::TDInfo	TDInfo;

ui2DDefSurvInfoDlg( uiParent* p, TDInfo ztyp )
    : uiDialog(p,uiDialog::Setup(tr("Survey setup for 2D only"), sDlgTitle(),
                                 mODHelpKey(m2DDefSurvInfoDlgHelpID) ))
    , tdinf_(ztyp)
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
				      BoolInpSpec(true,uiStrings::sMeter(false),
				      uiStrings::sFeet(false)) );
    ismfld_->attach( alignedBelow, yrgfld_ );

    uiSeparator* optsep = new uiSeparator( this, "Optional" );
    optsep->attach( stretchedBelow, maingrp );

    uiLabel* zrglbl = new uiLabel( this, tr("Optional:") );
    zrglbl->attach( leftBorder );
    zrglbl->attach( ensureBelow, optsep );

    uiGroup* optgrp = new uiGroup( this, "Optional parameters" );

    const uiString zunitlbl( tdinf_ == uiSurvInfoProvider::Time
	    ? uiStrings::sTimeUnitString()
	    : uiStrings::sDistUnitString(tdinf_!=uiSurvInfoProvider::Depth) );
    zmaxfld_ = new uiGenInput( optgrp,
	       tr("Z-max").withUnit(zunitlbl).optional(), fis );
    srfld_ = new uiGenInput( optgrp,
	     tr("Default sampling rate").withUnit(zunitlbl).optional(), fis );
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


bool acceptOK()
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

    const float defzmax = tdinf_ == uiSurvInfoProvider::Time ? cDefaultTWTMax
      : ( tdinf_ == uiSurvInfoProvider::Depth ? cDefaultZMaxft : cDefaultZMaxm);
    if ( mIsUdf(zmaxfld_->getFValue()) )
	zmaxfld_->setValue( defzmax );

    const float defsr = tdinf_ == uiSurvInfoProvider::Time ? cDefautSRms
	 : ( tdinf_ == uiSurvInfoProvider::Depth ? cDefautSRft : cDefautSRm );
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

virtual TDInfo tdInfo( bool& isknown ) const
{ isknown = true; return tdinf_; }

    const TDInfo	tdinf_;
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


uiDialog* ui2DSurvInfoProvider::dialog( uiParent* p, TDInfo ztyp )
{
    return new ui2DDefSurvInfoDlg( p, ztyp );
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
    if ( c0.x_ > c1.x_ )
	std::swap( c0.x_, c1.x_ );
    if ( c0.y_ > c1.y_ )
	std::swap( c0.y_, c1.y_ );
    const Coord d( c1.x_ - c0.x_, c1.y_ - c0.y_ );
    const double grdsp = dlg->grdspfld_->getDValue();
    const int nrinl = (int)(d.x_ / grdsp + 1.5);
    const int nrcrl = (int)(d.y_ / grdsp + 1.5);
    if ( nrinl < 2 && nrcrl < 2 )
    {
	gUiMsg(dlg).error(
		tr("Coordinate ranges are less than one trace distance"));
	return false;
    }

    cs.hsamp_.start_.inl() = cs.hsamp_.start_.crl() = 10000;
    cs.hsamp_.step_.inl() = cs.hsamp_.step_.crl() = 1;
    cs.hsamp_.stop_.inl() = 10000 + nrinl - 1;
    cs.hsamp_.stop_.crl() = 10000 + nrcrl -1;

    Coord cmax( c0.x_ + grdsp*(nrinl-1), c0.y_ + grdsp*(nrcrl-1) );
    if ( cmax.x_ < c0.x_ )
	std::swap( cmax.x_, c0.x_ );
    if ( cmax.y_ < c0.y_ )
	std::swap( cmax.y_, c0.y_ );
    crd[0] = c0;
    crd[1] = cmax;
    crd[2] = Coord( c0.x_, cmax.y_ );

    const float zfac = SI().showZ2UserFactor();
    cs.zsamp_.start = 0.f;
    cs.zsamp_.stop = dlg->zmaxfld_->getFValue() / zfac;
    cs.zsamp_.step = dlg->srfld_->getFValue() / zfac;

    xyft_ = !dlg->ismfld_->getBoolValue();

    return true;
}



// uiCopySurveySIP
class uiSurveyToCopyDlg : public uiDialog
{ mODTextTranslationClass(uiSurveyToCopyDlg)
public:

uiSurveyToCopyDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Select Survey to duplicate Setup from"),
		mNoDlgTitle,mODHelpKey(mCopySurveySIPHelpID)))
{
    survsel_ = new uiSurveySelect( this );
}

    uiSurveySelect* survsel_;

};


uiDialog* uiCopySurveySIP::dialog( uiParent* p, TDInfo )
{
    return new uiSurveyToCopyDlg( p );
}


bool uiCopySurveySIP::getInfo( uiDialog* dlg, TrcKeyZSampling& cs, Coord crd[3])
{
    tdinf_ = Time; xyinft_ = false; tdinfknown_ = false;
    mDynamicCastGet(uiSurveyToCopyDlg*,seldlg,dlg)
    if ( !seldlg )
	return false;

    const BufferString survdir = seldlg->survsel_->getFullDirPath();
    uiRetVal uirv;
    PtrMan<SurveyInfo> survinfo = SurveyInfo::read( survdir, uirv );
    if ( !survinfo )
	return false;

    survinfo->getSampling( cs );
    crd[0] = survinfo->transform( cs.hsamp_.start_ );
    crd[1] = survinfo->transform( cs.hsamp_.stop_ );
    crd[2] = survinfo->transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()));

    tdinf_ = survinfo->zIsTime() ? Time
				 : (survinfo->zInFeet() ? DepthFeet : Depth);
    xyinft_ = survinfo->xyInFeet();
    tdinfknown_ = true;
    coordsystem_ = survinfo->getCoordSystem();

    return true;
}


IOPar* uiCopySurveySIP::getCoordSystemPars() const
{
    if ( !coordsystem_ )
	return 0;

    IOPar* crspar = new IOPar;
    coordsystem_->fillPar( *crspar );
    return crspar;
}



//uiSurveyFileSIP
uiSurveyFileSIP::uiSurveyFileSIP()
    : tdinfknown_(false)
    , xyinft_(false)
{}


uiString uiSurveyFileSIP::usrText() const
{
    return tr("Read from Survey Setup file");
}


class uiSurveyFileDlg : public uiDialog
{ mODTextTranslationClass(uiSurveyFileDlg)
public:

uiSurveyFileDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Select Survey Setup file"),
			mNoDlgTitle, mTODOHelpKey))
{
    uiFileSel::Setup fssu;
    fssu.withexamine( true )
	.initialselectiondir( GetBaseDataDir() )
	.allowallextensions( true );
    inpfld_ = new uiFileSel( this, uiStrings::sSelect(), fssu );
}

    uiFileSel*	inpfld_;

};


uiDialog* uiSurveyFileSIP::dialog( uiParent* p, TDInfo )
{
    return new uiSurveyFileDlg( p );
}


bool uiSurveyFileSIP::getInfo( uiDialog* dlg, TrcKeyZSampling& cs, Coord crd[3])
{
    tdinf_ = Time;
    tdinfknown_ = false;
    xyinft_ = false;
    mDynamicCastGet(uiSurveyFileDlg*,filedlg,dlg)
    if ( !filedlg ) return false;

    const BufferString fname = filedlg->inpfld_->fileName();
    IOPar pars;
    if ( !pars.read(fname,"Survey Info",true) )
	return false;

    PtrMan<SurveyInfo> survinfo = new SurveyInfo();
    survinfo->usePar( pars );

    survinfo->getSampling( cs );
    crd[0] = survinfo->transform( cs.hsamp_.start_ );
    crd[1] = survinfo->transform( cs.hsamp_.stop_ );
    crd[2] = survinfo->transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()));

    tdinf_ = survinfo->zIsTime() ? Time
				 : (survinfo->zInFeet() ? DepthFeet : Depth);
    tdinfknown_ = true;
    xyinft_ = survinfo->xyInFeet();
    coordsystem_ = survinfo->getCoordSystem();

    return true;
}


IOPar* uiSurveyFileSIP::getCoordSystemPars() const
{
    if ( !coordsystem_ )
	return 0;

    IOPar* crspar = new IOPar;
    coordsystem_->fillPar( *crspar );
    return crspar;
}
