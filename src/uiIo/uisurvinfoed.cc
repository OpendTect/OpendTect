/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uisurvinfoed.h"
#include "uisip.h"

#include "cubesampling.h"
#include "cubesubsel.h"
#include "trckeyzsampling.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "iopar.h"
#include "oddirs.h"
#include "ptrman.h"
#include "statrand.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "unitofmeasure.h"
#include "latlong.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicoordsystem.h"
#include "uifileselector.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uisurvmap.h"
#include "uitabstack.h"
#include "od_helpids.h"


uiSurvInfoProvider::TDInfo uiSurvInfoProvider::getTDInfo(
			    bool istime, bool zinft )
{
    TDInfo ztyp = Time;
    if ( istime )
	ztyp = zinft ? DepthFeet : Depth;
    return ztyp;
}



bool uiSurvInfoProvider::runDialog( uiParent* p, TDInfo ztyp, SurveyInfo& si,
			      bool defdpthinft, bool* havezinfo )
{
    PtrMan<uiDialog> dlg = dialog( p, ztyp );
    if ( !dlg || !dlg->go() )
	return false;

    TrcKeyZSampling cs; Coord crd[3];
    if ( !getInfo(dlg,cs,crd) )
	return false;

    PtrMan<IOPar> crspar = getCoordSystemPars();
    RefMan<Coords::CoordSystem> coordsys =
		crspar ? Coords::CoordSystem::createSystem( *crspar ) : 0;
    if ( coordsys )
	si.setCoordSystem( coordsys );

    const bool xyinfeet = si.xyInFeet();
    bool tdinfknown = false;
    uiSurvInfoProvider::TDInfo tdinfo = tdInfo( tdinfknown );
    bool zistime = si.zIsTime();
    if ( tdinfknown )
	zistime = tdinfo == uiSurvInfoProvider::Time;
    bool zinfeet = defdpthinft;
    if ( zistime )
    {
	if ( xyinfeet )
	    zinfeet = true;
    }
    else
	zinfeet = tdinfo == uiSurvInfoProvider::DepthFeet;

    si.setZUnit( zistime, zinfeet );
    IOPar defpars( si.getDefaultPars() );
    defpars.setYN( SurveyInfo::sKeyDpthInFt(), zinfeet );
    si.setDefaultPars( defpars, false );

    float srd = 0.f;
    if ( getSRD(srd) && !mIsUdf(srd) )
    {
	if ( !zistime && zinfeet )
	    srd *= mToFeetFactorF;
	si.setSeismicReferenceDatum( srd );
    }

    const bool havez = !mIsUdf(cs.zsamp_.start);
    if ( !havez )
	cs.zsamp_ = si.zRange();

    si.setRanges( CubeSampling(cs) );
    si.setWorkRanges( CubeSubSel(cs) );
    BinID bid[2];
    bid[0].inl() = cs.hsamp_.start_.inl();
    bid[0].crl() = cs.hsamp_.start_.crl();
    bid[1].inl() = cs.hsamp_.stop_.inl();
    bid[1].crl() = cs.hsamp_.stop_.crl();
    si.set3Pts( crd, bid, cs.hsamp_.stop_.crl() );
    return true;
}


uiString uiSurveyInfoEditor::getSRDString( bool infeet )
{
    uiString ret( toUiString(SurveyInfo::sKeySeismicRefDatum()) );
    if ( infeet )
	ret.appendPlainText( " " ); // to keep at same length
    ret.withUnit( uiStrings::sDistUnitString(infeet,true) );
    return ret;
}


static void setZValFld( uiGenInput* zfld, int nr, float val, float fac )
{
    if ( mIsUdf(val) )
	{ zfld->setText( "", nr ); return; }

    val *= fac; int ival = mNINT32(val); float fival = mCast(float,ival);
    if ( mIsEqual(val,fival,0.01) )
	zfld->setValue( ival, nr );
    else
	zfld->setValue( val, nr );
}

uiSurveyInfoEditor::uiSurveyInfoEditor( uiParent* p, bool isnewborn )
	: uiDialog(p,uiDialog::Setup(tr("Edit Survey Parameters"),
				     mNoDlgTitle,
				     mODHelpKey(mSurveyInfoEditorHelpID) )
				     .nrstatusflds(1))
	, basepath_(SI().basePath())
	, orgdirname_(SI().dirName())
	, si_(const_cast<SurveyInfo&>(SI()))
	, x0fld_(0)
	, sipfld_(0)
	, lastsip_(0)
	, impiop_(0)
	, gengrp_(0)
{
    BufferString storagedir = si_.getFullDirPath();
    if ( File::isLink(storagedir) )
    {
	BufferString newstoragedir = File::linkEnd(storagedir);
	File::Path fp( newstoragedir );
	if ( !fp.isAbsolute() )
	{
	    fp.setPath( File::Path(storagedir).pathOnly() );
	    newstoragedir = File::linkEnd( fp.fullPath() );
	}
	storagedir = newstoragedir;
    }
    File::Path fp( storagedir );
    basepath_ = fp.pathOnly();
    orgdirname_ = fp.fileName();

    gengrp_ = new uiGroup( this, "Top group" );
    survnmfld_ = new uiGenInput( gengrp_, tr("Survey name"),
				StringInpSpec(si_.name()) );

    uiGroup* survmapgrp = new uiGroup( this, "SurvMap group" );
    surveymap_ = new uiSurveyMap( survmapgrp );
    inlgridview_ = new uiGrid2DMapObject();
    inlgridview_->setLineStyle( OD::LineStyle(OD::LineStyle::Dot) );
    inlgrid_ = new Grid2D();
    inlgridview_->setGrid( inlgrid_ );
    surveymap_->addObject( inlgridview_ );
    // does not work:
    // surveymap_->attachGroup().setPrefWidth( 300 );
    // surveymap_->attachGroup().setPrefHeight( 200 );
    // survmapgrp->setPrefWidth( 300 );
    survmapgrp->attachObj()->setMinimumWidth( 300 );
    survmapgrp->setFrame( true );

    pathfld_ = new uiGenInput( gengrp_, tr("Location on disk"),
				StringInpSpec(basepath_) );
    pathfld_->attach( alignedBelow, survnmfld_ );

#ifdef __win__
    pathfld_->setSensitive( false );
#else
    uiButton* pathbut = uiButton::getStd( gengrp_, OD::Select,
			      mCB(this,uiSurveyInfoEditor,pathbutPush), false );
    pathbut->attach( rightOf, pathfld_ );
#endif

    uiLabeledComboBox* lcb = new uiLabeledComboBox( gengrp_,
			SurveyInfo::Pol2D3DDef(), tr("Survey type") );
    lcb->attach( alignedBelow, pathfld_ ); pol2d3dfld_ = lcb->box();

    mkSIPFld( lcb->attachObj(), isnewborn );
    if ( sipfld_ )
	gengrp_->setHAlignObj( sipfld_->box() );
    else
	gengrp_->setHAlignObj( lcb );

    const float srd = si_.seismicReferenceDatum();
    const bool zistime = si_.zDomain().isTime();
    const bool depthinft = si_.depthsInFeet();
    const UnitOfMeasure* datauom = zistime || !depthinft
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    const UnitOfMeasure* displayuom = !depthinft ? UnitOfMeasure::meterUnit()
						 : UnitOfMeasure::feetUnit();
    refdatumfld_ = new uiGenInput( gengrp_, getSRDString(depthinft),
		       FloatInpSpec(getConvertedValue(srd,datauom,displayuom)));
    refdatumfld_->attach( alignedBelow, sipfld_  );

    uiGroup* topgrp = new uiGroup( this, "Top group" );
    uiSplitter* versplit = new uiSplitter( topgrp );
    versplit->addGroup( gengrp_ );
    versplit->addGroup( survmapgrp );

    tabs_ = new uiTabStack( this, "Survey setup" );

    mkRangeGrp();

    mkCoordGrp();

    mkTransfGrp();

    mkLatLongGrp();

    uiSplitter* horsplit = new uiSplitter( this, "Hor split", OD::Horizontal );
    horsplit->addGroup( topgrp );
    horsplit->addGroup( tabs_ );

    postFinalise().notify( mCB(this,uiSurveyInfoEditor,doFinalise) );
    sipCB(0);
}


uiSurveyInfoEditor::~uiSurveyInfoEditor()
{
    delete impiop_;
    delete inlgrid_;
}


void uiSurveyInfoEditor::mkSIPFld( uiObject* att, bool isnewborn )
{
    sips_ = survInfoProvs();
    for ( int idx=0; idx<sips_.size(); idx++ )
    {
	if ( !sips_[idx]->isAvailable() )
	    { sips_.removeSingle(idx); idx--; }
    }
    const int nrprovs = sips_.size();
    if ( nrprovs < 1 ) return;

    sipfld_ = new uiLabeledComboBox( gengrp_, tr("Ranges/coordinate settings"));
    sipfld_->attach( alignedBelow, att );
    uiComboBox* box = sipfld_->box();
    box->addItem( tr("Enter below") );
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	uiSurvInfoProvider& sip = *sips_[idx];
	box->addItem( sip.usrText() );
	const char* icnm = sip.iconName();
	if ( icnm && *icnm )
	    box->setIcon( box->size()-1, icnm );
    }
    box->setCurrentItem( 0 );

    const CallBack sipcb = mCB(this,uiSurveyInfoEditor,sipCB);
    const uiString sipnm = si_.sipName();
    if ( !sipnm.isEmpty() )
    {
	const int sipidx = box->indexOf( sipnm );
	if ( sipidx < 0 )
	{
	    if ( isnewborn )
		uiMSG().error( tr("'%1' is not available.\nThis is probably "
				    "a license issue").arg( sipnm ) );
	}
	else
	{
	    box->setCurrentItem( sipidx );
	    if ( isnewborn )
		postFinalise().notify( sipcb );
	}
    }
    box->selectionChanged.notify( sipcb );
}


void uiSurveyInfoEditor::mkRangeGrp()
{
    rangegrp_ = new uiGroup( tabs_->tabGroup(), "Survey ranges" );

    const Interval<int> startstoprg( -mUdf(int), mUdf(int) );
    const Interval<int> steprg( 1, mUdf(int) );
    IntInpIntervalSpec iis( true );
    iis.setLimits( startstoprg, -1 ).setLimits( steprg, 2 );
    iis.setName("Inl Start",0).setName("Inl Stop",1).setName("Inl step",2);
    uiLabel* emptyspace = new uiLabel( rangegrp_, uiString::empty() );
    inlfld_ = new uiGenInput( rangegrp_,
			      uiStrings::phrInline(uiStrings::sRange()), iis );
    inlfld_->valuechanged.notify( mCB(this,uiSurveyInfoEditor,rangeChg) );
    inlfld_->attach( alignedBelow, emptyspace );
    nrinlslbl_ = new uiLabel( rangegrp_, uiString::empty() );
    nrinlslbl_->setStretch( 2, 0 );
    nrinlslbl_->attach( rightTo, inlfld_ );

    iis.setName("Crl Start",0).setName("Crl Stop",1).setName("Crl step",2);
    crlfld_ = new uiGenInput( rangegrp_, tr("Cross-line range"), iis );
    crlfld_->valuechanged.notify( mCB(this,uiSurveyInfoEditor,rangeChg) );
    nrcrlslbl_ = new uiLabel( rangegrp_, uiString::empty() );
    nrcrlslbl_->setStretch( 2, 0 );
    nrcrlslbl_->attach( rightTo, crlfld_ );

    zfld_ = new uiGenInput( rangegrp_, tr("Z range"),
			   DoubleInpIntervalSpec(true).setName("Z Start",0)
						      .setName("Z Stop",1)
						      .setName("Z step",2) );
    zfld_->valuechanged.notify( mCB(this,uiSurveyInfoEditor,rangeChg) );
    crlfld_->attach( alignedBelow, inlfld_ );
    zfld_->attach( alignedBelow, crlfld_ );

    const bool zistime = si_.zDomain().isTime();
    const bool depthinft = si_.depthsInFeet();

    uiStringSet zunitstrs;
    zunitstrs.add( uiStrings::sMSec(false,mPlural).toLower() );
    zunitstrs.add( uiStrings::sMeter(false).toLower() );
    zunitstrs.add( uiStrings::sFeet(false).toLower() );
    zunitfld_ = new uiComboBox( rangegrp_, zunitstrs, "Z unit" );
    zunitfld_->attach( rightOf, zfld_ );
    zunitfld_->setCurrentItem( zistime ? 0 : depthinft ? 2 : 1 );
    zunitfld_->selectionChanged.notify( mCB(this,uiSurveyInfoEditor,updZUnit));

    depthdispfld_ = new uiGenInput( rangegrp_, tr("Display depths in"),
	   BoolInpSpec(!depthinft, uiStrings::sMeter(false),
			uiStrings::sFeet(false)) );
    depthdispfld_->setSensitive( zistime && !si_.xyInFeet() );
    depthdispfld_->valuechanged.notify(
			mCB(this,uiSurveyInfoEditor,depthDisplayUnitSel) );
    depthdispfld_->attach( alignedBelow, zfld_ );



    rangegrp_->setHAlignObj( inlfld_ );
    tabs_->addTab( rangegrp_ );
    tabs_->setTabIcon( rangegrp_, "alldirs" ); //TODO
}



void uiSurveyInfoEditor::mkCoordGrp()
{
    #define mAddCB( fld )\
	mAttachCB( fld->valuechanged, uiSurveyInfoEditor::coordsChg );

    crdgrp_ = new uiGroup( tabs_->tabGroup(), "Survey Grid (Easy)" );
    PositionInpSpec::Setup psetup;
    uiLabel* emptyspace = new uiLabel( crdgrp_, uiString::empty() );

    ic0fld_ = new uiGenInput( crdgrp_, tr("First In-line/Cross-line"),
		     PositionInpSpec(psetup).setName("Inl Position1",0)
					    .setName("Crl Position1",1) );
    ic0fld_->attach( alignedBelow, emptyspace );
    ic0fld_->valuechanging.notify( mCB(this,uiSurveyInfoEditor,setInl1Fld) );
    ic1fld_ = new uiGenInput( crdgrp_, tr("Another position on above In-line"),
		     PositionInpSpec(psetup).setName("Inl Position2",0)
					    .setName("Crl Position2",1) );
    ic2fld_ = new uiGenInput( crdgrp_, tr("Position not on above In-line"),
		      PositionInpSpec(psetup).setName("Inl Position3",0)
					     .setName("Crl Position3",1) );
    mAddCB( ic2fld_ );

    psetup.wantcoords_ = true;
    uiString xystr = tr("= (X,Y)");
    xy0fld_ = new uiGenInput( crdgrp_, xystr,
				PositionInpSpec(psetup).setName("X1",0)
						       .setName("Y1",1) );
    xy0fld_->setElemSzPol( uiObject::SmallVar );
    mAddCB( xy0fld_ );

    xy1fld_ = new uiGenInput( crdgrp_, xystr,
				PositionInpSpec(psetup).setName("X2",0)
						       .setName("Y2",1) );
    xy1fld_->setElemSzPol( uiObject::SmallVar );
    mAddCB( xy1fld_ );

    xy2fld_ = new uiGenInput( crdgrp_, xystr,
				PositionInpSpec(psetup).setName("X3",0)
						       .setName("Y3",1) );
    xy2fld_->setElemSzPol( uiObject::SmallVar );
    mAddCB( xy2fld_ );

    ic1fld_->attach( alignedBelow, ic0fld_ );
    ic2fld_->attach( alignedBelow, ic1fld_ );
    xy0fld_->attach( rightOf, ic0fld_ );
    xy1fld_->attach( rightOf, ic1fld_ );
    xy2fld_->attach( rightOf, ic2fld_ );

    crdgrp_->setHAlignObj( ic0fld_ );
    tabs_->addTab( crdgrp_ );
    tabs_->setTabIcon( crdgrp_, "gridsettings-easy" ); //TODO
#undef mAddCB
}


void uiSurveyInfoEditor::mkTransfGrp()
{
    #define mAddCB( fld )\
	mAttachCB( fld->valuechanged, uiSurveyInfoEditor::transformChg );

    trgrp_ = new uiGroup( tabs_->tabGroup(), "Survey Grid (Advanced)" );
    uiLabel* emptyspace = new uiLabel( trgrp_, uiString::empty() );

    x0fld_ = new uiGenInput( trgrp_, tr("%1 = ").arg( uiStrings::sX() ),
					   DoubleInpSpec().setName( "X" ) );
    x0fld_->setElemSzPol( uiObject::SmallVar );
    x0fld_->attach( alignedBelow, emptyspace );
    mAddCB( x0fld_ );

    xinlfld_ = new uiGenInput( trgrp_, tr("+ %1 *").arg(uiStrings::sInline()),
				       DoubleInpSpec().setName("Inl") );
    xinlfld_->setElemSzPol( uiObject::SmallVar );
    mAddCB( xinlfld_ );

    xcrlfld_ = new uiGenInput( trgrp_,tr("+ %1 *").arg(uiStrings::sCrossline()),
			       DoubleInpSpec().setName("Crl") );
    xcrlfld_->setElemSzPol( uiObject::SmallVar );
    mAddCB( xcrlfld_ );

    y0fld_ = new uiGenInput ( trgrp_, tr("Y = "), DoubleInpSpec().setName("Y"));
    y0fld_->setElemSzPol( uiObject::SmallVar );
    mAddCB( y0fld_ );

    yinlfld_ = new uiGenInput( trgrp_, tr("+ %1 *").arg(uiStrings::sInline()),
			       DoubleInpSpec() .setName("Inl"));
    yinlfld_->setElemSzPol( uiObject::SmallVar );
    mAddCB( yinlfld_ );

    ycrlfld_ = new uiGenInput( trgrp_,tr("+ %1 *").arg(uiStrings::sCrossline()),
			       DoubleInpSpec().setName("Crl"));
    ycrlfld_->setElemSzPol( uiObject::SmallVar );
    overrulefld_ = new uiCheckBox( trgrp_, tr("Overrule easy settings") );
    mAddCB( ycrlfld_ );

    overrulefld_->setChecked( false );
    overrulefld_->activated.notify( mCB(this,uiSurveyInfoEditor,overruleCB) );
    xinlfld_->attach( rightOf, x0fld_ );
    xcrlfld_->attach( rightOf, xinlfld_ );
    y0fld_->attach( alignedBelow, x0fld_ );
    yinlfld_->attach( rightOf, y0fld_ );
    ycrlfld_->attach( rightOf, yinlfld_ );
    overrulefld_->attach( alignedBelow, ycrlfld_ );
    trgrp_->setHAlignObj( xinlfld_ );
    tabs_->addTab( trgrp_ );
    tabs_->setTabIcon( trgrp_, "gridsettings-advanced" ); //TODO
}


void uiSurveyInfoEditor::mkLatLongGrp()
{
    latlonggrp_ = new uiGroup( tabs_->tabGroup(), sKey::CoordSys() );
    uiLabel* emptyspace = new uiLabel( latlonggrp_, uiString::empty() );
    latlongsel_ = new Coords::uiCoordSystemSelGrp( latlonggrp_, true, false,
						&si_, si_.getCoordSystem() );
    latlongsel_->attach( alignedBelow, emptyspace );
    tabs_->addTab( latlonggrp_ );
    tabs_->setTabIcon( latlonggrp_, "spherewire" );
}


void uiSurveyInfoEditor::overruleCB( CallBacker* )
{
    crdgrp_->setSensitive( !overrulefld_->isChecked() );
}


void uiSurveyInfoEditor::setValues()
{
    TrcKeyZSampling cs( false );
    si_.getSampling( cs );
    const TrcKeySampling& hs = cs.hsamp_;
    StepInterval<int> inlrg( hs.start_.inl(), hs.stop_.inl(), hs.step_.inl() );
    StepInterval<int> crlrg( hs.start_.crl(), hs.stop_.crl(), hs.step_.crl() );
    inlfld_->setValue( inlrg );
    crlfld_->setValue( crlrg );
    updateLabels();

    const StepInterval<float>& zrg = si_.zRange();
    const float zfac = mCast( float, si_.zDomain().userFactor() );
    setZValFld( zfld_, 0, zrg.start, zfac );
    setZValFld( zfld_, 1, zrg.stop, zfac );
    setZValFld( zfld_, 2, zrg.step, zfac );

    x0fld_->setValue( si_.b2c_.getTransform(true).a );
    xinlfld_->setValue( si_.b2c_.getTransform(true).b );
    xcrlfld_->setValue( si_.b2c_.getTransform(true).c );
    y0fld_->setValue( si_.b2c_.getTransform(false).a );
    yinlfld_->setValue( si_.b2c_.getTransform(false).b );
    ycrlfld_->setValue( si_.b2c_.getTransform(false).c );

    Coord c[3]; BinID b[2]; int xline;
    si_.get3Pts( c, b, xline );
    if ( b[0].inl() )
    {
	ic0fld_->setValue( b[0] );
	ic1fld_->setValues( b[0].inl(), xline );
	ic2fld_->setValue( b[1] );
	if ( !c[0].x_ && !c[0].y_ &&
	     !c[1].x_ && !c[1].y_ &&
	     !c[2].x_ && !c[2].y_ )
	{
	    c[0] = si_.transform( b[0] );
	    c[1] = si_.transform( b[1] );
	    c[2] = si_.transform( BinID(b[0].inl(),xline) );
	}
	xy0fld_->setValue( c[0] );
	xy1fld_->setValue( c[2] );
	xy2fld_->setValue( c[1] );
    }

    const bool zistime = si_.zDomain().isTime();
    const bool xyinfeet = si_.xyInFeet();
    const bool zinfeet = si_.depthsInFeet();
    zunitfld_->setCurrentItem( zistime	? 0 : (zinfeet ? 2 : 1) );
    depthdispfld_->setValue( !zinfeet );
    depthdispfld_->setSensitive( zistime && !xyinfeet );

    const float srd = si_.seismicReferenceDatum();
    const UnitOfMeasure* datauom = zistime || !zinfeet
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    const UnitOfMeasure* displayuom = depthdispfld_->getBoolValue()
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    refdatumfld_->setValue( getConvertedValue( srd, datauom, displayuom ) );
    updateMap();
}


void uiSurveyInfoEditor::updateLabels()
{
    const StepInterval<int> irg( inlfld_->getIStepInterval() );
    const StepInterval<int> crg( crlfld_->getIStepInterval() );
    nrinlslbl_->setText( tr("Nr. In-lines: %1").arg(irg.nrSteps()+1) );
    nrcrlslbl_->setText( tr("Nr. Cross-lines: %1").arg(crg.nrSteps()+1) );
}


ObjectSet<uiSurvInfoProvider>& uiSurveyInfoEditor::survInfoProvs()
{
    mDefineStaticLocalObject( PtrMan<ObjectSet<uiSurvInfoProvider> >, sips,
			      = new ObjectSet<uiSurvInfoProvider> );
    return *sips;
}


int uiSurveyInfoEditor::addInfoProvider( uiSurvInfoProvider* p )
{
    if ( p ) survInfoProvs() += p;
    return survInfoProvs().size();
}


bool uiSurveyInfoEditor::renameSurv( const char* path, const char* indirnm,
				     const char* outdirnm )
{
    const BufferString fnmin = File::Path(path).add(indirnm).fullPath();
    const BufferString fnmout = File::Path(path).add(outdirnm).fullPath();
    if ( File::exists(fnmout) )
    {
	uiString msg = tr("Cannot rename %1 to %2"
			  "\nbecause target directory exists")
		     .arg(fnmin).arg(fnmout);
	gUiMsg().error( msg );
	return false;
    }
    File::rename( fnmin, fnmout );
    if ( !File::exists(fnmout) )
    {
	uiString msg = tr("Rename %1 to %2 failed\n"
			  "See startup window for details")
		     .arg(fnmin).arg(fnmout);
	gUiMsg().error( msg );
	return false;
    }

    return true;
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSurveyInfoEditor::getFromScreen()
{
    if ( !setSurvName() || !( setInlCrlRange() && setZRange() ) )
	mErrRet( errmsg_ );

    if ( !overrulefld_->isChecked() )
    {
	if ( !setCoords() )
	    mErrRet( errmsg_ );

	x0fld_->setValue( si_.b2c_.getTransform(true).a );
	xinlfld_->setValue( si_.b2c_.getTransform(true).b );
	xcrlfld_->setValue( si_.b2c_.getTransform(true).c );
	y0fld_->setValue( si_.b2c_.getTransform(false).a );
	yinlfld_->setValue( si_.b2c_.getTransform(false).b );
	ycrlfld_->setValue( si_.b2c_.getTransform(false).c );
	overrulefld_->setChecked( false );
    }
    else if ( !setRelation() )
	mErrRet( errmsg_ );

    si_.updateGeometries();
    return true;
}


void uiSurveyInfoEditor::doFinalise( CallBacker* )
{
    pathfld_->setText( basepath_ );
    pathfld_->setReadOnly( true );
    updStatusBar( basepath_ );

    pol2d3dfld_->setCurrentItem( (int)si_.survDataType() );

    TrcKeyZSampling cs( false );
    si_.getSampling( cs );
    const TrcKeySampling& hs = cs.hsamp_;
    if ( hs.totalNr() > 0 )
	setValues();
}


bool uiSurveyInfoEditor::setSurvName()
{
    BufferString newsurvnm( survnmfld_->text() );
    if ( newsurvnm.size() < 2 )
    {
	errmsg_ = uiStrings::phrSpecify(tr("a valid survey name"));
	return false;
    }
    si_.setName( newsurvnm );
    return true;
}


bool uiSurveyInfoEditor::acceptOK()
{
    if ( !getFromScreen() )
	return false;

    const BufferString newbasepath( pathfld_->text() );
    const BufferString newdirnm( dirName() );
    const BufferString olddir = File::Path( basepath_ ).add( orgdirname_ )
					.fullPath();
    const BufferString newdir = File::Path( newbasepath, newdirnm )
					.fullPath();
    const bool storepathchanged = basepath_ != newbasepath;
    bool dirnamechanged = orgdirname_ != newdirnm;

    if ( (dirnamechanged || storepathchanged) && File::exists(newdir) )
    {
	uiMSG().error( tr("The new target directory exists.\n"
		       "Please enter another survey name or location.") );
	return false;
    }

    if ( storepathchanged )
    {
	if ( !uiMSG().askGoOn(tr("Copy your survey to another location?")) )
	    return false;

	BufferString olddirnm = orgdirname_;
	if ( !dirnamechanged )
	{
	    olddirnm.add( "_org" );
	    if ( !renameSurv(basepath_,orgdirname_,olddirnm) )
		return false;
	}

	SurveyInfo* newsi = uiSurvey::copySurvey( this, si_.name(),
				basepath_, olddirnm, newbasepath );
	if ( !newsi )
	{
	    if ( dirnamechanged )
		renameSurv( basepath_, olddirnm, orgdirname_ );
	    return false;
	}

	si_ = *newsi;
	getFromScreen();
	delete newsi;

	if ( !uiMSG().askGoOn(tr("Keep the survey (suffixed '_org') "
				    "at the old location?")) )
	    File::remove( olddirnm );

	basepath_ = newbasepath;
    }
    else if ( dirnamechanged )
    {
	if ( !renameSurv(basepath_,orgdirname_,newdirnm) )
	    return false;
    }

    si_.diskloc_.set( File::Path(basepath_,newdirnm) );
    si_.setSurvDataType( (SurveyInfo::Pol2D3D)pol2d3dfld_->currentItem() );
    if ( overrulefld_->isChecked() )
	si_.get3Pts( si_.set3coords_, si_.set3binids_,
			si_.set3binids_[2].crl() );

    if ( lastsip_ )
	si_.setSipName( lastsip_->usrText() );

    if ( si_.isFresh() )
    {
	IOPar iop;
	si_.getFreshSetupData( iop );
	if ( lastsip_ )
	    iop.set( uiSurvInfoProvider::sKeySIPName(),
		     toString(lastsip_->usrText()) );
	if ( impiop_ )
	    iop.merge( *impiop_ );
	si_.setFreshSetupData( iop );
    }

    if ( latlongsel_->acceptOK() && latlongsel_->outputSystem() )
	si_.setCoordSystem( latlongsel_->outputSystem() );

    if ( !si_.write() )
	mErrRet( tr("Failed to write survey info.\nNo changes committed.") );

    return true;
}


BufferString uiSurveyInfoEditor::dirName() const
{
    return SurveyInfo::dirNameForName( survnmfld_->text() );
}


#define mErrRetTabGrp(grp,err) \
    { tabs_->setCurrentPage(grp); errmsg_ = err; return false; }


bool uiSurveyInfoEditor::setInlCrlRange()
{
    #define mStopNotif( fld ) \
    NotifyStopper ns##fld( fld->valuechanged );

    mStopNotif( inlfld_ );
    mStopNotif( crlfld_ );

    const StepInterval<int> irg( inlfld_->getIStepInterval() );
    const StepInterval<int> crg( crlfld_->getIStepInterval() );
    if ( irg.isUdf() ) mErrRetTabGrp(rangegrp_,uiStrings::phrEnter(tr(
						  "a valid range for inlines")))
    if ( crg.isUdf() ) mErrRetTabGrp(rangegrp_,uiStrings::phrEnter(tr(
					       "a valid range for crosslines")))
    TrcKeyZSampling cs( false );
    si_.getSampling( cs );
    TrcKeySampling& hs = cs.hsamp_;
    hs.start_.inl() = irg.start; hs.start_.crl() = crg.start;
    hs.stop_.inl() = irg.atIndex( irg.getIndex(irg.stop) );
    hs.stop_.crl() = crg.atIndex( crg.getIndex(crg.stop) );
    hs.step_.inl() = irg.step;	 hs.step_.crl() = crg.step;
    if ( hs.step_.inl() < 1 ) hs.step_.inl() = 1;
    if ( hs.step_.crl() < 1 ) hs.step_.crl() = 1;

     if ( !hs.totalNr() )
	mErrRetTabGrp(rangegrp_,
			uiStrings::phrSpecify(tr("in-line/cross-line ranges")))

    si_.setRanges( CubeSampling(cs) );
    si_.setWorkRanges( CubeSubSel(cs) );
    return true;
}


bool uiSurveyInfoEditor::setZRange()
{
    const bool zistime = zunitfld_->currentItem() == 0;
    const bool zinfeet = !depthdispfld_->getBoolValue();

    si_.setZUnit( zistime, zinfeet );
    si_.defpars_.setYN( SurveyInfo::sKeyDpthInFt(), zinfeet );

    const float srd = refdatumfld_->getFValue();
    const UnitOfMeasure* datauom = zistime || !zinfeet
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    const UnitOfMeasure* displayuom = !zinfeet
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    si_.setSeismicReferenceDatum( getConvertedValue(srd,displayuom,datauom) );

    TrcKeyZSampling cs( false );
    si_.getSampling( cs );
    cs.zsamp_ = zfld_->getFStepInterval();
    if (mIsUdf(cs.zsamp_.start) || mIsUdf(cs.zsamp_.stop)
				|| mIsUdf(cs.zsamp_.step))
	mErrRetTabGrp( rangegrp_, uiStrings::phrEnter(uiStrings::sZRange()) );

    const float zfac = 1.f / si_.zDomain().userFactor();
    if ( !mIsEqual(zfac,1,0.0001) )
	cs.zsamp_.scale( zfac );

    if ( mIsZero(cs.zsamp_.step,1e-8) )
	cs.zsamp_.step = si_.zDomain().isTime() ? 0.004f : 1;

    cs.normalise();
    if ( cs.zsamp_.nrSteps() == 0 )
	mErrRetTabGrp( rangegrp_, uiStrings::phrSpecify(tr("a valid Z range")))

    si_.setRanges( CubeSampling(cs) );
    si_.setWorkRanges( CubeSubSel(cs) );
    return true;
}


bool uiSurveyInfoEditor::setCoords()
{
#define mStopNotif( fld ) \
    NotifyStopper ns##fld( fld->valuechanged );

    mStopNotif( ic0fld_ );
    mStopNotif( ic1fld_ );
    mStopNotif( ic2fld_ );
    mStopNotif( xy0fld_ );
    mStopNotif( xy1fld_ );
    mStopNotif( xy2fld_ );

    BinID b[2]; Coord c[3]; int xline;
    b[0] = ic0fld_->getBinID();
    b[1] = ic2fld_->getBinID();
    xline = ic1fld_->getBinID().crl();
    c[0] = xy0fld_->getCoord();
    c[1] = xy2fld_->getCoord();
    c[2] = xy1fld_->getCoord();

    const uiString errmsg = si_.set3Pts( c, b, xline );
    if ( !errmsg.isEmpty() )
	mErrRetTabGrp( crdgrp_, errmsg )
    else if ( overrulefld_->isChecked() )
	si_.gen3Pts();

    return true;
}


bool uiSurveyInfoEditor::setRelation()
{
    #define mStopNotif( fld ) \
    NotifyStopper ns##fld( fld->valuechanged );

    mStopNotif( x0fld_ );
    mStopNotif( y0fld_ );
    mStopNotif( xinlfld_ );
    mStopNotif( yinlfld_ );
    mStopNotif( xcrlfld_ );
    mStopNotif( ycrlfld_ );

    Pos::IdxPair2Coord::DirTransform xtr, ytr;
    xtr.a = x0fld_->getDValue();   ytr.a = y0fld_->getDValue();
    xtr.b = xinlfld_->getDValue(); ytr.b = yinlfld_->getDValue();
    xtr.c = xcrlfld_->getDValue(); ytr.c = ycrlfld_->getDValue();
    if ( !xtr.valid(ytr) )
	mErrRetTabGrp( trgrp_, tr("The transformation is not valid.") );

    si_.b2c_.setTransforms( xtr, ytr );
    return true;
}


void uiSurveyInfoEditor::sipCB( CallBacker* cb )
{
    const int sipidx = sipfld_ ? sipfld_->box()->currentItem() : 0;
    if ( sipidx < 1 )
	return;
    sipfld_->box()->setCurrentItem( 0 );
    delete impiop_; impiop_ = 0; lastsip_ = 0;

    const bool zistime = zunitfld_->currentItem() == 0;
    const bool zinfeet = !depthdispfld_->getBoolValue();
    uiSurvInfoProvider* sip = sips_[sipidx-1];
    bool havez = false;
    if ( !sip->runDialog(this,uiSurvInfoProvider::getTDInfo(zistime,zinfeet),
			    si_,zinfeet,&havez) )
	return;

    setValues();
    if ( !havez )
	zfld_->clear();

    lastsip_ = sip;
    impiop_ = lastsip_->getImportPars();
    updateMap();
}


void uiSurveyInfoEditor::pathbutPush( CallBacker* )
{
    uiFileSelector::Setup fssu( pathfld_->text() );
    fssu.selectDirectory();
    uiFileSelector uifs( this, fssu );
    if ( uifs.go() )
    {
	BufferString dirnm( uifs.fileName() );
	if ( !File::isWritable(dirnm) )
	{
	    uiMSG().error( tr("Directory is not writable") );
	    return;
	}
	updStatusBar( dirnm );
	pathfld_->setText( dirnm );
    }
}


void uiSurveyInfoEditor::updStatusBar( const char* dirnm )
{
    uiString msg;
    System::getFreeMBOnDiskMsg( System::getFreeMBOnDisk(dirnm), msg );
    toStatusBar( msg );
}


void uiSurveyInfoEditor::setInl1Fld( CallBacker* )
{
    ic1fld_->setText( ic0fld_->text(0), 0 );
}


void uiSurveyInfoEditor::rangeChg( CallBacker* cb )
{
    if ( cb == inlfld_ )
    {
	StepInterval<int> irg = inlfld_->getIStepInterval();
	if ( mIsUdf(irg.step) || !irg.step || irg.step>irg.width() )irg.step=1;
	if ( irg.isUdf() ) return;

	irg.stop = irg.atIndex( irg.getIndex(irg.stop) );
	inlfld_->setValue( irg );
	setInlCrlRange();
    }
    else if ( cb == crlfld_ )
    {
	StepInterval<int> crg = crlfld_->getIStepInterval();
	if ( mIsUdf(crg.step) || !crg.step || crg.step>crg.width() )crg.step=1;
	if ( crg.isUdf() ) return;

	crg.stop = crg.atIndex( crg.getIndex(crg.stop) );
	crlfld_->setValue( crg );
	setInlCrlRange();
    }
    else if ( cb == zfld_ )
    {
	StepInterval<double> zrg = zfld_->getDStepInterval();
	if ( mIsUdf(zrg.step) || mIsZero(zrg.step,1e-6) ) return;

	zrg.stop = zrg.atIndex( zrg.getIndex(zrg.stop) );
	zfld_->setValue( zrg );
	setZRange();
    }

    updateLabels();
    updateMap();
}


void uiSurveyInfoEditor::coordsChg( CallBacker* )
{
    setCoords();
    updateMap();
}


void uiSurveyInfoEditor::transformChg( CallBacker* )
{
    setRelation();
    updateMap();
}


void uiSurveyInfoEditor::updateMap()
{
    TypeSet<int> inlines, crlines;
    si_.updateGeometries();
    TrcKeyZSampling cs( false );
    si_.getSampling( cs );
    const TrcKeySampling& hs = cs.hsamp_;
    const int inlstep = ( hs.nrInl() * hs.step_.inl() ) / 5;
    for ( int idx=0; idx<4; idx++ )
    {
	const int inl = hs.start_.inl() + (idx+1) * inlstep;
	inlines += inl;
    }

    inlgrid_->set( inlines, crlines, hs );
    inlgridview_->setGrid( inlgrid_ );
    surveymap_->setSurveyInfo( &si_ );
}


void uiSurveyInfoEditor::depthDisplayUnitSel( CallBacker* )
{
    const bool showdepthinft = !depthdispfld_->getBoolValue();
    refdatumfld_->setTitleText( getSRDString(showdepthinft) );
    float refdatum = refdatumfld_->getFValue();
    refdatum *= showdepthinft ? mToFeetFactorF : mFromFeetFactorF;
    refdatumfld_->setValue( refdatum );
}


void uiSurveyInfoEditor::updZUnit( CallBacker* cb )
{
    const UnitOfMeasure* prevdisplayuom = depthdispfld_->getBoolValue()
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    const float oldsrduser = refdatumfld_->getFValue();

    const bool zintime = zunitfld_->currentItem() == 0;
    const bool zinft = zunitfld_->currentItem() == 2;

    depthdispfld_->setSensitive( zintime );
    if ( zintime )
    {
	depthdispfld_->setValue( false );
    }
    else
	depthdispfld_->setValue( !zinft );

    const bool showdepthinft = !depthdispfld_->getBoolValue();
    const UnitOfMeasure* newdisplayuom = !showdepthinft
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    const float newsrduser = getConvertedValue( oldsrduser, prevdisplayuom,
						newdisplayuom );
    refdatumfld_->setValue( newsrduser );
    refdatumfld_->setTitleText( getSRDString(showdepthinft) );
    const UnitOfMeasure* datauom = zintime || !zinft
		     ? UnitOfMeasure::meterUnit() : UnitOfMeasure::feetUnit();
    si_.setSeismicReferenceDatum( getConvertedValue(newsrduser,newdisplayuom,
						    datauom) );
}
