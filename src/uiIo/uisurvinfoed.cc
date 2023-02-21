/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurvinfoed.h"
#include "uisip.h"

#include "bufstringset.h"
#include "coordsystem.h"
#include "trckeyzsampling.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iopar.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "ptrman.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "unitofmeasure.h"
#include "latlong.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicoordsystem.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisurveyselect.h"
#include "uitabstack.h"
#include "od_helpids.h"

extern "C" const char* GetBaseDataDir();

uiString uiSurveyInfoEditor::getSRDString( bool infeet )
{
    uiString lbl = uiString( tr("%1%2%3") )
		 .arg( SurveyInfo::sKeySeismicRefDatum() )
		 .arg( infeet ? "  " : " " ) //to keep string always same length
		 .arg( getDistUnitString(infeet,true) );

    return lbl;
}


uiString uiSurveyInfoEditor::getCoordString( bool infeet )
{
    uiString txt = tr("Coordinates are in %1%2")
			.arg( getDistUnitString(infeet,false)).arg("  ");
    return txt;
}


uiSurveyInfoEditor::uiSurveyInfoEditor( uiParent* p, SurveyInfo& si,
					bool isnew )
	: uiDialog(p,uiDialog::Setup(tr("Edit Survey Parameters"),
				     mNoDlgTitle,
				     mODHelpKey(mSurveyInfoEditorHelpID))
					.nrstatusflds(1))
	, survParChanged(this)
	, si_(si)
	, orgdirname_(si_.getDirName().buf())
	, rootdir_(GetBaseDataDir())
	, isnew_(isnew)
	, impiop_(nullptr)
	, lastsip_(nullptr)
	, coordsystem_(si.getCoordSystem())
	, x0fld_(nullptr)
	, topgrp_(nullptr)
	, sipfld_(nullptr)
	, xyinftfld_(nullptr)
	, dirnamechanged(false)
{
    orgstorepath_ = si_.getDataDirName().buf();

    BufferString fulldirpath;
    if ( isnew_ )
    {
	fulldirpath = FilePath( rootdir_ ).add( orgdirname_ ).fullPath();
	SurveyInfo::pushSI( &si_ );
    }
    else
    {
	BufferString storagedir = FilePath(orgstorepath_).add(orgdirname_)
							 .fullPath();
	int linkcount = 0;
	while ( linkcount++ < 20 && File::isLink(storagedir) )
	{
	    BufferString newstoragedir = File::linkTarget(storagedir);
	    FilePath fp( newstoragedir );
	    if ( !fp.isAbsolute() )
	    {
		fp.setPath( FilePath(storagedir).pathOnly() );
		newstoragedir = fp.fullPath();
	    }
	    storagedir = newstoragedir;
	}
	if ( linkcount < 20 )
	{
	    FilePath fp( storagedir );
	    orgstorepath_ = fp.pathOnly();
	    orgdirname_ = fp.fileName();
	}

	fulldirpath = storagedir;
    }

    topgrp_ = new uiGroup( this, "Top group" );
    survnmfld_ = new uiGenInput( topgrp_, tr("Survey name"),
				StringInpSpec(si_.name()) );
    survnmfld_->setElemSzPol( uiObject::Wide );

    pathfld_ = new uiGenInput( topgrp_, tr("Location on disk"),
				StringInpSpec(orgstorepath_) );
    pathfld_->attach( alignedBelow, survnmfld_ );
    pathfld_->setElemSzPol( uiObject::Wide );

#ifdef __win__
    pathfld_->setSensitive( false );
#else
    uiButton* pathbut = uiButton::getStd( topgrp_, OD::Select,
			      mCB(this,uiSurveyInfoEditor,pathbutPush), false );
    pathbut->attach( rightOf, pathfld_ );
#endif

    uiLabeledComboBox* lcb = new uiLabeledComboBox( topgrp_,
				SurveyInfo::Pol2D3DNames(), tr("Survey type") );
    lcb->attach( alignedBelow, pathfld_ ); pol2dfld_ = lcb->box();

    mkSIPFld( lcb->attachObj() );
    if ( sipfld_ )
	topgrp_->setHAlignObj( sipfld_ );
    else
	topgrp_->setHAlignObj( lcb );

    uiSeparator* horsep1 = new uiSeparator( this, "Hor sep 1" );
    horsep1->attach( stretchedBelow, topgrp_, -2 );

    tabs_ = new uiTabStack( this, "Survey setup" );
    tabs_->attach( ensureBelow, horsep1 );
    mkRangeGrp();
    mkCoordGrp();
    mkTransfGrp();
    mkCRSGrp();

    uiButton* applybut = uiButton::getStd( this, OD::Apply,
			    mCB(this,uiSurveyInfoEditor,appButPushed), true );
    applybut->attach( centeredBelow, tabs_ );

    mAttachCB( afterPopup, uiSurveyInfoEditor::doFinalize );
}


uiSurveyInfoEditor::~uiSurveyInfoEditor()
{
    detachAllNotifiers();

    delete impiop_;
    if ( isnew_ )
	SurveyInfo::popSI();
}


void uiSurveyInfoEditor::mkSIPFld( uiObject* att )
{
    sips_ = survInfoProvs();
    for ( int idx=0; idx<sips_.size(); idx++ )
    {
	if ( !sips_[idx]->isAvailable() )
	    { sips_.removeSingle(idx); idx--; }
    }
    const int nrprovs = sips_.size();
    if ( nrprovs < 1 ) return;

    uiLabeledComboBox* lcb = new uiLabeledComboBox( topgrp_,
					    tr("Ranges/coordinate settings") );
    lcb->attach( alignedBelow, att );
    sipfld_ = lcb->box();
    sipfld_->addItem( tr("Enter below") );
    mAttachCB( sipfld_->selectionChanged, uiSurveyInfoEditor::sipCB );

    for ( int idx=0; idx<nrprovs; idx++ )
    {
	uiSurvInfoProvider& sip = *sips_[idx];
	sipfld_->addItem( mToUiStringTodo(sip.usrText()) );
	const char* icnm = sip.iconName();
	if ( icnm && *icnm )
	    sipfld_->setIcon( sipfld_->size()-1, icnm );
    }
    sipfld_->setCurrentItem( 0 );

    if ( !si_.sipName().isEmpty() )
    {
	const BufferString sipnm = si_.sipName();
	const int sipidx = sipfld_->indexOf( sipnm );
	if (sipidx >= 0)
	    sipfld_->setCurrentItem(sipidx);
	else
	    uiMSG().error( tr("The survey setup method is not available.\n"
			      "Probably, this is a license issue") );
    }
}


void uiSurveyInfoEditor::mkRangeGrp()
{
    rangegrp_ = new uiGroup( tabs_->tabGroup(), "Survey ranges" );

    uiLabel* dummy = new uiLabel( rangegrp_, uiStrings::sEmptyString() );

    const Interval<int> startstoprg( -mUdf(int), mUdf(int) );
    const Interval<int> steprg( 1, mUdf(int) );
    IntInpIntervalSpec iis( true );
    iis.setLimits( startstoprg, -1 ).setLimits( steprg, 2 );
    iis.setName("Inl Start",0).setName("Inl Stop",1).setName("Inl step",2);
    inlfld_ = new uiGenInput( rangegrp_, tr("In-line range"), iis );
    mAttachCB( inlfld_->valueChanged, uiSurveyInfoEditor::rangeChg );
    inlfld_->attach( alignedBelow, dummy );
    nrinlslbl_ = new uiLabel( rangegrp_, uiStrings::sEmptyString() );
    nrinlslbl_->setStretch( 2, 0 );
    nrinlslbl_->attach( rightTo, inlfld_ );

    iis.setName("Crl Start",0).setName("Crl Stop",1).setName("Crl step",2);
    crlfld_ = new uiGenInput( rangegrp_, tr("Cross-line range"), iis );
    mAttachCB( crlfld_->valueChanged, uiSurveyInfoEditor::rangeChg );
    nrcrlslbl_ = new uiLabel( rangegrp_, uiStrings::sEmptyString() );
    nrcrlslbl_->setStretch( 2, 0 );
    nrcrlslbl_->attach( rightTo, crlfld_ );

    zfld_ = new uiGenInput( rangegrp_, tr("Z range"),
			   DoubleInpIntervalSpec(true).setName("Z Start",0)
						      .setName("Z Stop",1)
						      .setName("Z step",2) );
    mAttachCB( zfld_->valueChanged, uiSurveyInfoEditor::rangeChg );
    crlfld_->attach( alignedBelow, inlfld_ );
    zfld_->attach( alignedBelow, crlfld_ );

    const bool zistime = si_.zDomain().isTime();
    const bool depthinft = si_.depthsInFeet();

    const char* zunitstrs[] = { "millisecond", "meter", "feet", 0 };
    zunitfld_ = new uiComboBox( rangegrp_, zunitstrs, "Z unit" );
    zunitfld_->attach( rightOf, zfld_ );
    zunitfld_->setCurrentItem( zistime ? 0 : depthinft ? 2 : 1 );
    mAttachCB( zunitfld_->selectionChanged, uiSurveyInfoEditor::updZUnit );

    depthdispfld_ = new uiGenInput( rangegrp_, tr("Display depths in"),
	BoolInpSpec(!depthinft,uiStrings::sMeter(),uiStrings::sFeet()) );
    depthdispfld_->setSensitive( zistime && !si_.xyInFeet() );
    mAttachCB( depthdispfld_->valueChanged,
	       uiSurveyInfoEditor::depthDisplayUnitSel );
    depthdispfld_->attach( alignedBelow, zfld_ );

    const float srd = si_.seismicReferenceDatum();
    const UnitOfMeasure* datauom = zistime || !depthinft
				 ? UoMR().get( "Meter" ) : UoMR().get( "Feet" );
    const UnitOfMeasure* displayuom = depthdispfld_->getBoolValue()
				    ? UoMR().get( "Meter" )
				    : UoMR().get( "Feet" );
    refdatumfld_ = new uiGenInput( rangegrp_, getSRDString(depthinft),
		       FloatInpSpec(getConvertedValue(srd,datauom,displayuom)));
    refdatumfld_->attach( alignedBelow, depthdispfld_ );

    rangegrp_->setHAlignObj( inlfld_ );
    tabs_->addTab( rangegrp_, tr("Survey ranges"), "alldirs" );
}


void uiSurveyInfoEditor::mkCoordGrp()
{
    const Interval<int> inlcrlvals( -mUdf(int), mUdf(int) );
    IntInpIntervalSpec iis;
    iis.setLimits( inlcrlvals, -1 );

    crdgrp_ = new uiGroup( tabs_->tabGroup(), "Coordinate settings" );
    uiLabel* dummy = new uiLabel( crdgrp_, uiStrings::sEmptyString() );
    iis.setName("Inl Position1",0).setName("Crl Position1",1);
    ic0fld_ = new uiGenInput( crdgrp_, tr("First In-line/Cross-line"), iis );
    mAttachCB( ic0fld_->valueChanging, uiSurveyInfoEditor::ic0ChgCB );
    ic0fld_->attach( alignedBelow, dummy );

    iis.setName("Inl Position2",0).setName("Crl Position2",1);
    ic1fld_ = new uiGenInput( crdgrp_, tr("Another position on above In-line"),
			      iis );

    iis.setName("Inl Position3",0).setName("Crl Position3",1);
    ic2fld_ = new uiGenInput( crdgrp_, tr("Position not on above In-line"),
			      iis );
    mAttachCB( ic2fld_->valueChanging, uiSurveyInfoEditor::ic2ChgCB );

    iis.setName("Inl Position4",0).setName("Crl Position4",1);
    ic3fld_ = new uiGenInput( crdgrp_, tr("Fourth position"), iis );

    PositionInpSpec::Setup psetup;
    psetup.wantcoords_ = true;
    uiString xystr = tr("= (X,Y)");
    xy0fld_ = new uiGenInput( crdgrp_, xystr,
				PositionInpSpec(psetup).setName("X1",0)
						       .setName("Y1",1) );
    xy0fld_->setElemSzPol( uiObject::SmallVar );
    xy1fld_ = new uiGenInput( crdgrp_, xystr,
				PositionInpSpec(psetup).setName("X2",0)
						       .setName("Y2",1) );
    xy1fld_->setElemSzPol( uiObject::SmallVar );
    xy2fld_ = new uiGenInput( crdgrp_, xystr,
				PositionInpSpec(psetup).setName("X3",0)
						       .setName("Y3",1) );
    xy2fld_->setElemSzPol( uiObject::SmallVar );
    xy3fld_ = new uiGenInput( crdgrp_, xystr,
				PositionInpSpec(psetup).setName("X4",0)
						       .setName("Y4",1) );
    xy3fld_->setElemSzPol( uiObject::SmallVar );

    ic1fld_->attach( alignedBelow, ic0fld_ );
    ic2fld_->attach( alignedBelow, ic1fld_ );
    ic3fld_->attach( alignedBelow, ic2fld_ );
    xy0fld_->attach( rightOf, ic0fld_ );
    xy1fld_->attach( rightOf, ic1fld_ );
    xy2fld_->attach( rightOf, ic2fld_ );
    xy3fld_->attach( rightOf, ic3fld_ );

    xyunitlbl_ = new uiLabel( crdgrp_, getCoordString(xyInFeet()) );
    xyunitlbl_->attach( alignedBelow, xy3fld_ );

    crdgrp_->setHAlignObj( ic0fld_ );
    tabs_->addTab( crdgrp_, tr("Coordinate settings"), "gridsettings-easy" );
}


void uiSurveyInfoEditor::mkTransfGrp()
{
    trgrp_ = new uiGroup( tabs_->tabGroup(), "I/C to X/Y transformation" );
    uiLabel* dummy = new uiLabel( trgrp_, uiStrings::sEmptyString() );
    x0fld_ = new uiGenInput ( trgrp_, tr("X = "), DoubleInpSpec().setName("X"));
    x0fld_->setElemSzPol( uiObject::SmallVar );
    x0fld_->attach( alignedBelow, dummy );

    xinlfld_ = new uiGenInput ( trgrp_, tr("+ %1 *").arg(uiStrings::sInline()),
				       DoubleInpSpec().setName("Inl") );
    xinlfld_->setElemSzPol( uiObject::SmallVar );
    xcrlfld_ = new uiGenInput ( trgrp_,
				tr("+ %1 *").arg(uiStrings::sCrossline()),
				DoubleInpSpec().setName("Crl") );
    xcrlfld_->setElemSzPol( uiObject::SmallVar );
    y0fld_ = new uiGenInput ( trgrp_, tr("Y = "), DoubleInpSpec().setName("Y"));
    y0fld_->setElemSzPol( uiObject::SmallVar );
    yinlfld_ = new uiGenInput ( trgrp_, tr("+ %1 *").arg(uiStrings::sInline()),
				      DoubleInpSpec() .setName("Inl"));
    yinlfld_->setElemSzPol( uiObject::SmallVar );
    ycrlfld_ = new uiGenInput ( trgrp_,
				tr("+ %1 *").arg(uiStrings::sCrossline()),
				DoubleInpSpec() .setName("Crl"));
    ycrlfld_->setElemSzPol( uiObject::SmallVar );
    overrulefld_ = new uiCheckBox( trgrp_, tr("Overrule easy settings") );
    overrulefld_->setChecked( false );
    xinlfld_->attach( rightOf, x0fld_ );
    xcrlfld_->attach( rightOf, xinlfld_ );
    y0fld_->attach( alignedBelow, x0fld_ );
    yinlfld_->attach( rightOf, y0fld_ );
    ycrlfld_->attach( rightOf, yinlfld_ );
    overrulefld_->attach( alignedBelow, ycrlfld_ );
    trgrp_->setHAlignObj( xinlfld_ );

    tabs_->addTab( trgrp_, tr("I/C to X/Y transformation"),
		   "gridsettings-advanced" );
}


void uiSurveyInfoEditor::mkCRSGrp()
{
    crsgrp_ = new uiGroup( tabs_->tabGroup(), sKey::CoordSys() );
    crssel_ = new Coords::uiCoordSystemSelGrp( crsgrp_, true, false,
						&si_, si_.getCoordSystem() );
    tabs_->addTab( crsgrp_, uiStrings::sCoordSys(), "crs" );
}


static void setZValFld( uiGenInput* zfld, int nr, float val, int nrdec )
{
    if ( mIsUdf(val) )
	{ zfld->setText( "", nr ); return; }

    BufferString valstr;
    valstr.set( val, nrdec );
    zfld->setText( valstr, nr );
}


void uiSurveyInfoEditor::setValues()
{
    const TrcKeyZSampling& cs = si_.sampling( false );
    const TrcKeySampling& hs = cs.hsamp_;
    StepInterval<int> inlrg( hs.start_.inl(), hs.stop_.inl(), hs.step_.inl() );
    StepInterval<int> crlrg( hs.start_.crl(), hs.stop_.crl(), hs.step_.crl() );
    inlfld_->setValue( inlrg );
    crlfld_->setValue( crlrg );
    updateLabels();

    const StepInterval<float>& zrg = si_.zRange( false );
    const float zfac = sCast( float, si_.zDomain().userFactor() );
    setZValFld( zfld_, 0, zrg.start*zfac, si_.nrZDecimals() );
    setZValFld( zfld_, 1, zrg.stop*zfac, si_.nrZDecimals() );
    setZValFld( zfld_, 2, zrg.step*zfac, si_.nrZDecimals() );

    int nrdec = 4;
    x0fld_->setValue( si_.b2c_.getTransform(true).a );
    x0fld_->setNrDecimals( nrdec );
    xinlfld_->setValue( si_.b2c_.getTransform(true).b );
    xinlfld_->setNrDecimals( nrdec );
    xcrlfld_->setValue( si_.b2c_.getTransform(true).c );
    xcrlfld_->setNrDecimals( nrdec );
    y0fld_->setValue( si_.b2c_.getTransform(false).a );
    y0fld_->setNrDecimals( nrdec );
    yinlfld_->setValue( si_.b2c_.getTransform(false).b );
    yinlfld_->setNrDecimals( nrdec );
    ycrlfld_->setValue( si_.b2c_.getTransform(false).c );
    ycrlfld_->setNrDecimals( nrdec );

    Coord c[3]; BinID b[2]; int xline;
    si_.get3Pts( c, b, xline );
    if ( !b[0].isUdf() )
    {
	ic0fld_->setValue( b[0] );
	ic1fld_->setValues( b[0].inl(), xline );
	ic2fld_->setValue( b[1] );
	ic3fld_->setValues( b[1].inl(), b[0].crl() );
	if ( !c[0].x && !c[0].y && !c[1].x && !c[1].y && !c[2].x && !c[2].y)
	{
	    c[0] = si_.transform( b[0] );
	    c[1] = si_.transform( b[1] );
	    c[2] = si_.transform( BinID(b[0].inl(),xline) );
	}

	const Coord c4 = si_.b2c_.transform( ic3fld_->getBinID() );
	xy0fld_->setValue( c[0] );
	xy1fld_->setValue( c[2] );
	xy2fld_->setValue( c[1] );
	xy3fld_->setValue( c4 );
	nrdec = si_.nrXYDecimals();
	xy0fld_->setNrDecimals( nrdec, 0 ); xy0fld_->setNrDecimals( nrdec, 1 );
	xy1fld_->setNrDecimals( nrdec, 0 ); xy1fld_->setNrDecimals( nrdec, 1 );
	xy2fld_->setNrDecimals( nrdec, 0 ); xy2fld_->setNrDecimals( nrdec, 1 );
	xy3fld_->setNrDecimals( nrdec, 0 ); xy3fld_->setNrDecimals( nrdec, 1 );
    }

    const bool zistime = si_.zDomain().isTime();
    const bool xyinfeet = si_.xyInFeet();
    const bool zinfeet = si_.depthsInFeet();
    zunitfld_->setCurrentItem( zistime	? 0 : (zinfeet ? 2 : 1) );
    depthdispfld_->setValue( !zinfeet );
    depthdispfld_->setSensitive( zistime && !xyinfeet );

    const float srd = si_.seismicReferenceDatum();
    const UnitOfMeasure* datauom = zistime || !zinfeet ? UoMR().get( "Meter" )
						       : UoMR().get( "Feet" );
    const UnitOfMeasure* displayuom = depthdispfld_->getBoolValue()
				    ? UoMR().get( "Meter" )
				    : UoMR().get( "Feet" );
    refdatumfld_->setValue( getConvertedValue( srd, datauom, displayuom ) );
}


void uiSurveyInfoEditor::updateLabels()
{
    const StepInterval<int> irg( inlfld_->getIStepInterval() );
    const StepInterval<int> crg( crlfld_->getIStepInterval() );
    nrinlslbl_->setText( tr("Nr. In-lines: %1").arg(irg.nrSteps()+1) );
    nrcrlslbl_->setText( tr("Nr. Cross-lines: %1").arg(crg.nrSteps()+1) );
}


static void deleteSIPS()
{
    deepErase( uiSurveyInfoEditor::survInfoProvs() );
}


ObjectSet<uiSurvInfoProvider>& uiSurveyInfoEditor::survInfoProvs()
{
    static PtrMan<ObjectSet<uiSurvInfoProvider> > sips;
    if ( !sips )
    {
	auto* newsips = new ObjectSet<uiSurvInfoProvider>;
	if ( sips.setIfNull(newsips,true) )
	    NotifyExitProgram( &deleteSIPS );
    }
    return *sips;
}


int uiSurveyInfoEditor::addInfoProvider( uiSurvInfoProvider* p )
{
    if ( p ) survInfoProvs() += p;
    return survInfoProvs().size();
}


bool uiSurveyInfoEditor::copySurv( const char* inpath, const char* indirnm,
				   const char* outpath, const char* outdirnm )
{
    const BufferString fnmin = FilePath(inpath).add(indirnm).fullPath();
    const BufferString fnmout = FilePath(outpath).add(outdirnm).fullPath();
    if ( File::exists(fnmout) )
    {
	uiString msg = tr("Cannot copy %1 to %2"
			  "\nbecause target folder exists.")
		     .arg(fnmin).arg(fnmout);
	uiMSG().error( msg );
	return false;
    }
    MouseCursorManager::setOverride( MouseCursor::Wait );
    File::copy( fnmin, fnmout );
    MouseCursorManager::restoreOverride();
    if ( !File::exists(fnmout) )
    {
	uiString msg = tr("Copy %1 to %2 failed\n"
			  "See startup window for details")
		     .arg(fnmin).arg(fnmout);
	uiMSG().error( msg );
	return false;
    }

    return true;
}


bool uiSurveyInfoEditor::renameSurv( const char* path, const char* indirnm,
				     const char* outdirnm )
{
    const BufferString fnmin = FilePath(path).add(indirnm).fullPath();
    const BufferString fnmout = FilePath(path).add(outdirnm).fullPath();
    if ( File::exists(fnmout) )
    {
	uiString msg = tr("Cannot rename %1 to %2"
			  "\nbecause target folder exists.")
		     .arg(fnmin).arg(fnmout);
	uiMSG().error( msg );
	return false;
    }
    File::rename( fnmin, fnmout );
    if ( !File::exists(fnmout) )
    {
	uiString msg = tr("Rename %1 to %2 failed.\n"
			  "See startup window for details.")
		     .arg(fnmin).arg(fnmout);
	uiMSG().error( msg );
	return false;
    }

    return true;
}


#define mUseAdvanced() \
    (overrulefld_->isChecked() && tabs_->currentPage()==trgrp_)

void uiSurveyInfoEditor::appButPushed( CallBacker* )
{
    doApply();
}


bool uiSurveyInfoEditor::doApply()
{
    if ( !setSurvName() || !setRanges() )
	return false;

    if ( crssel_->acceptOK() && crssel_->outputSystem() )
    {
	coordsystem_ = crssel_->outputSystem();
	updZUnit( nullptr );
	xyunitlbl_->setText( getCoordString(xyInFeet()) );
    }

    if ( !mUseAdvanced() )
    {
	if ( !setCoords() )
	    return false;

	const int nrdec = 4;
	x0fld_->setValue( si_.b2c_.getTransform(true).a );
	x0fld_->setNrDecimals( nrdec );
	xinlfld_->setValue( si_.b2c_.getTransform(true).b );
	xinlfld_->setNrDecimals( nrdec );
	xcrlfld_->setValue( si_.b2c_.getTransform(true).c );
	xcrlfld_->setNrDecimals( nrdec );
	y0fld_->setValue( si_.b2c_.getTransform(false).a );
	y0fld_->setNrDecimals( nrdec );
	yinlfld_->setValue( si_.b2c_.getTransform(false).b );
	yinlfld_->setNrDecimals( nrdec );
	ycrlfld_->setValue( si_.b2c_.getTransform(false).c );
	ycrlfld_->setNrDecimals( nrdec );
	overrulefld_->setChecked( false );
    }
    else if ( !setRelation() )
	return false;

    si_.update3DGeometry();
    survParChanged.trigger();
    return true;
}


void uiSurveyInfoEditor::doFinalize( CallBacker* )
{
    pathfld_->setText( orgstorepath_ );
    pathfld_->setReadOnly( true );
    updStatusBar( orgstorepath_ );

    pol2dfld_->setCurrentItem( (int)si_.survDataType() );

    if ( si_.sampling(false).hsamp_.totalNr() )
	setValues();

    ic1fld_->setReadOnly( true, 0, 0 );
    ic3fld_->setReadOnly( true );
    xy3fld_->setReadOnly( true );

    sipCB(nullptr);
    updZUnit(nullptr);
}


bool uiSurveyInfoEditor::rejectOK( CallBacker* )
{
    if ( isnew_ )
    {
	const BufferString dirnm = FilePath(orgstorepath_).add(orgdirname_)
							  .fullPath();
	if ( File::exists(dirnm) )
	    File::remove( dirnm );
    }

    return true;
}


bool uiSurveyInfoEditor::setSurvName()
{
    BufferString newsurvnm( survnmfld_->text() );
    if ( newsurvnm.size() < 2 )
    {
	uiMSG().error( tr("Please specify a valid survey name") );
	return false;
    }

    si_.setName( newsurvnm );
    return true;
}


bool uiSurveyInfoEditor::acceptOK( CallBacker* )
{
    if ( !doApply() )
	return false;

    const BufferString newstorepath( pathfld_->text() );
    const BufferString newdirnm( dirName() );
    const BufferString olddir(
			FilePath(orgstorepath_).add(orgdirname_).fullPath() );
    const BufferString newdir(FilePath(newstorepath).add(newdirnm).fullPath());
    const bool storepathchanged = orgstorepath_ != newstorepath;
    dirnamechanged = orgdirname_ != newdirnm;

    if ( (dirnamechanged || storepathchanged) && File::exists(newdir) )
    {
	uiMSG().error( tr("The new target folder exists.\n"
		       "Please enter another survey name or location.") );
	return false;
    }

    if ( storepathchanged )
    {
	if ( !uiMSG().askGoOn(tr("Copy your survey to another location?")) )
	    return false;
	else if ( !copySurv(orgstorepath_,orgdirname_,
			    newstorepath,newdirnm) )
	    return false;
	else if ( !uiMSG().askGoOn(tr("Keep the survey at the old location?")) )
	    File::remove( olddir );
    }
    else if ( dirnamechanged )
    {
	if ( !renameSurv(orgstorepath_,orgdirname_,newdirnm) )
	    return false;
    }

    BufferString linkpos = FilePath(rootdir_).add(newdirnm).fullPath();
    if ( File::exists(linkpos) )
    {
       if ( File::isLink(linkpos) )
	   File::remove( linkpos );
    }

    if ( !File::exists(linkpos) )
    {
	if ( !File::createLink(newdir,linkpos) )
	{
	    uiString msg =
		uiStrings::phrCannotCreate( tr("link from \n%1 to \n%2")
					     .arg(newdir).arg(linkpos));
	    uiMSG().error( msg );
	    return false;
	}
    }

    si_.disklocation_.setDirName( newdirnm );
    si_.setSurvDataType( sCast(OD::Pol2D3D,pol2dfld_->currentItem()) );
    if ( mUseAdvanced() )
	si_.get3Pts( si_.set3coords_, si_.set3binids_,
			si_.set3binids_[2].crl() );

    if ( !si_.write(rootdir_) )
    {
	uiMSG().error(
	    tr("Failed to write survey info.\nNo changes committed.") );
	return false;
    }

    uiSurvInfoProvider* sip = getSIP();
    if ( isnew_ && sip && sip->hasSurveyImportDlg() )
    {
	const bool ret = uiMSG().askGoOn( tr("Proceed to import data?") );

	if ( ret )
	    sip->launchSurveyImportDlg( this->parent() )->go();
    }

    return true;
}


const char* uiSurveyInfoEditor::dirName() const
{
    mDeclStaticString( ret ); ret = survnmfld_->text();
    ret.clean( BufferString::AllowDots );
    return ret.buf();
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSurveyInfoEditor::setRanges()
{
    const StepInterval<int> irg( inlfld_->getIStepInterval() );
    const StepInterval<int> crg( crlfld_->getIStepInterval() );
    if ( irg.isUdf() ) mErrRet(tr("Please enter a valid range for inlines"))
    if ( crg.isUdf() ) mErrRet(tr("Please enter a valid range for crosslines"))
    TrcKeyZSampling cs( si_.sampling(false) );
    TrcKeySampling& hs = cs.hsamp_;
    hs.start_.inl() = irg.start; hs.start_.crl() = crg.start;
    hs.stop_.inl() = irg.atIndex( irg.getIndex(irg.stop) );
    hs.stop_.crl() = crg.atIndex( crg.getIndex(crg.stop) );
    hs.step_.inl() = irg.step;	 hs.step_.crl() = crg.step;
    if ( hs.step_.inl() < 1 ) hs.step_.inl() = 1;
    if ( hs.step_.crl() < 1 ) hs.step_.crl() = 1;

    const bool zistime = zunitfld_->currentItem() == 0;
    const bool zinfeet = !depthdispfld_->getBoolValue();
    si_.setZUnit( zistime, zinfeet );
    si_.getPars().setYN( SurveyInfo::sKeyDpthInFt(), zinfeet );

    const float srd = refdatumfld_->getFValue();
    const UnitOfMeasure* datauom = zistime || !zinfeet ? UoMR().get( "Meter" )
						       : UoMR().get( "Feet" );
    const UnitOfMeasure* displayuom = !zinfeet ? UoMR().get( "Meter" )
					       : UoMR().get( "Feet" );
    si_.setSeismicReferenceDatum( getConvertedValue(srd,displayuom,datauom) );

    cs.zsamp_ = zfld_->getFStepInterval();
    if (mIsUdf(cs.zsamp_.start) || mIsUdf(cs.zsamp_.stop)
				|| mIsUdf(cs.zsamp_.step))
	mErrRet(tr("Please enter the Z Range"))
    const float zfac = 1.f / si_.zDomain().userFactor();
    if ( !mIsEqual(zfac,1,0.0001) )
	cs.zsamp_.scale( zfac );

    if ( mIsZero(cs.zsamp_.step,1e-8) )
	cs.zsamp_.step = si_.zDomain().isTime() ? 0.004f : 1;
    cs.normalize();
    if ( !hs.totalNr() )
	mErrRet(tr("Please specify in-line/cross-line ranges"))
    if ( cs.zsamp_.nrSteps() == 0 )
	mErrRet(tr("Please specify a valid Z range"))

    si_.setRange( cs, false );
    si_.setRange( cs, true );
    return true;
}


bool uiSurveyInfoEditor::setCoords()
{
    BinID b[2]; Coord c[3]; int xline;
    b[0] = ic0fld_->getBinID();
    b[1] = ic2fld_->getBinID();
    xline = ic1fld_->getBinID().crl();
    c[0] = xy0fld_->getCoord();
    c[1] = xy2fld_->getCoord();
    c[2] = xy1fld_->getCoord();

    const uiString msg = si_.set3PtsWithMsg( c, b, xline );
    if ( !msg.isEmpty() ) { uiMSG().error( msg ); return false; }
    else if ( mUseAdvanced() )
	si_.gen3Pts();

    if ( coordsystem_ )
	si_.setCoordSystem( coordsystem_ );

    return true;
}


bool uiSurveyInfoEditor::setRelation()
{
    Pos::IdxPair2Coord::DirTransform xtr, ytr;
    xtr.a = x0fld_->getDValue();   ytr.a = y0fld_->getDValue();
    xtr.b = xinlfld_->getDValue(); ytr.b = yinlfld_->getDValue();
    xtr.c = xcrlfld_->getDValue(); ytr.c = ycrlfld_->getDValue();
    if ( !xtr.valid(ytr) )
    {
	uiMSG().error( tr("The transformation is not valid.") );
	return false;
    }

    si_.b2c_.setTransforms( xtr, ytr );
    return true;
}


void uiSurveyInfoEditor::updatePar( CallBacker* )
{}


void uiSurveyInfoEditor::sipCB( CallBacker* )
{
    const int sipidx = sipfld_ ? sipfld_->currentItem() : 0;
    if ( sipidx < 1 )
	return;

    sipfld_->setCurrentItem( 0 );
    deleteAndNullPtr( impiop_ );
    lastsip_ = nullptr;

    uiSurvInfoProvider* sip = sips_[sipidx-1];
    PtrMan<uiDialog> dlg = sip->dialog( this );
    if ( !dlg || !dlg->go() )
	return;

    TrcKeyZSampling cs; Coord crd[3];
    if ( !sip->getInfo(dlg,cs,crd) )
	return;

    IOPar& pars = si_.getLogPars();
    sip->fillLogPars( pars );
    PtrMan<IOPar> crspar = sip->getCoordSystemPars();
    RefMan<Coords::CoordSystem> coordsys =
		crspar ? Coords::CoordSystem::createSystem( *crspar ) : 0;

    if ( !coordsys )
    {
	Coord llcrd; LatLong llll;
	if ( sip->getLatLongAnchor(llcrd,llll) )
	{
	    RefMan<Coords::AnchorBasedXY> anchoredsystem =
				new Coords::AnchorBasedXY( llll, llcrd );
	    anchoredsystem->setIsFeet( sip->xyInFeet() );
	    coordsys = anchoredsystem;
	}
	else
	{
	    RefMan<Coords::UnlocatedXY> crs = new Coords::UnlocatedXY();
	    crs->setIsFeet( sip->xyInFeet() );
	    coordsys = crs;
	}
    }

    if ( coordsys )
    {
	si_.setCoordSystem( coordsys );
	coordsystem_ = coordsys;
	crssel_->fillFromSI();
    }

    const bool xyinfeet = si_.getCoordSystem()->isFeet();
    uiSurvInfoProvider::TDInfo tdinfo = sip->tdInfo();
    bool zistime = si_.zIsTime();
    if ( tdinfo != uiSurvInfoProvider::Uknown )
	zistime = tdinfo == uiSurvInfoProvider::Time;

    bool zinfeet = !depthdispfld_->getBoolValue();
    if ( zistime )
    {
	if ( xyinfeet )
	    zinfeet = true;
    }
    else if ( tdinfo != uiSurvInfoProvider::Uknown )
	zinfeet = tdinfo == uiSurvInfoProvider::DepthFeet;

    si_.setZUnit( zistime, zinfeet );
    si_.setXYInFeet( xyinfeet );
    xyunitlbl_->setText( getCoordString(xyInFeet()) );

    float srd = 0.f;
    if ( sip->getSRD(srd) && !mIsUdf(srd) )
    {
	if ( !zistime && zinfeet )
	    srd *= mToFeetFactorF;
	si_.setSeismicReferenceDatum( srd );
    }

    const bool havez = !mIsUdf(cs.zsamp_.start);
    if ( !havez )
	cs.zsamp_ = si_.zRange(false);

    si_.setRange(cs,false);
    BinID bid[2];
    bid[0].inl() = cs.hsamp_.start_.inl();
    bid[0].crl() = cs.hsamp_.start_.crl();
    bid[1].inl() = cs.hsamp_.stop_.inl();
    bid[1].crl() = cs.hsamp_.stop_.crl();
    si_.set3PtsWithMsg( crd, bid, cs.hsamp_.stop_.crl() );
    setValues();
    if ( !havez )
	zfld_->clear();

    lastsip_ = sip;
    impiop_ = lastsip_->getImportPars();
}


void uiSurveyInfoEditor::pathbutPush( CallBacker* )
{
    uiFileDialog dlg( this, uiFileDialog::DirectoryOnly, pathfld_->text() );
    if ( dlg.go() )
    {
	BufferString dirnm( dlg.fileName() );
	if ( !File::isWritable(dirnm) )
	{
	    uiMSG().error( tr("Folder is not writable") );
	    return;
	}
	updStatusBar( dirnm );
	pathfld_->setText( dirnm );
    }
}


void uiSurveyInfoEditor::updStatusBar( const char* dirnm )
{
    BufferString msg;
    System::getFreeMBOnDiskMsg( System::getFreeMBOnDisk(dirnm), msg );
    toStatusBar( mToUiStringTodo(msg) );
}


void uiSurveyInfoEditor::ic0ChgCB( CallBacker* )
{
    const BinID bid = ic0fld_->getBinID();
    ic1fld_->setValue( bid.inl(), 0 );
    ic3fld_->setValue( bid.crl(), 1 );
}


void uiSurveyInfoEditor::ic2ChgCB( CallBacker* )
{
    ic3fld_->setValue( ic2fld_->getIntValue(0), 0 );
}


bool uiSurveyInfoEditor::xyInFeet() const
{
    return coordsystem_ ? coordsystem_->isFeet() : false;
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
    }
    else if ( cb == crlfld_ )
    {
	StepInterval<int> crg = crlfld_->getIStepInterval();
	if ( mIsUdf(crg.step) || !crg.step || crg.step>crg.width() )crg.step=1;
	if ( crg.isUdf() ) return;

	crg.stop = crg.atIndex( crg.getIndex(crg.stop) );
	crlfld_->setValue( crg );
    }
    else if ( cb == zfld_ )
    {
	StepInterval<double> zrg = zfld_->getDStepInterval();
	if ( mIsUdf(zrg.step) || mIsZero(zrg.step,1e-6) ) return;

	zrg.stop = zrg.atIndex( zrg.getIndex(zrg.stop) );
	zfld_->setValue( zrg );
    }

    updateLabels();
}


void uiSurveyInfoEditor::depthDisplayUnitSel( CallBacker* )
{
    const bool showdepthinft = !depthdispfld_->getBoolValue();
    refdatumfld_->setTitleText( getSRDString(showdepthinft) );
    float refdatum = refdatumfld_->getFValue();
    refdatum *= showdepthinft ? mToFeetFactorF : mFromFeetFactorF;
    refdatumfld_->setValue( refdatum );
}


void uiSurveyInfoEditor::updZUnit( CallBacker* )
{
    const UnitOfMeasure* prevdisplayuom = depthdispfld_->getBoolValue()
					? UoMR().get( "Meter" )
					: UoMR().get( "Feet" );
    const float oldsrduser = refdatumfld_->getFValue();

    const bool zintime = zunitfld_->currentItem() == 0;
    const bool zinft = zunitfld_->currentItem() == 2;
    const bool xyinft = xyInFeet();
    depthdispfld_->setSensitive( zintime && !xyinft );
    if ( zintime )
    {
	if ( xyinft )
	    depthdispfld_->setValue( false );
    }
    else
	depthdispfld_->setValue( !zinft );

    const bool showdepthinft = !depthdispfld_->getBoolValue();
    const UnitOfMeasure* newdisplayuom = !showdepthinft ? UoMR().get( "Meter" )
							: UoMR().get( "Feet" );
    const float newsrduser = getConvertedValue( oldsrduser, prevdisplayuom,
						newdisplayuom );
    refdatumfld_->setValue( newsrduser );
    refdatumfld_->setTitleText( getSRDString(showdepthinft) );
    const UnitOfMeasure* datauom = zintime || !zinft ? UoMR().get( "Meter" )
						     : UoMR().get( "Feet" );
    si_.setSeismicReferenceDatum( getConvertedValue(newsrduser,newdisplayuom,
						    datauom) );
}


void uiSurveyInfoEditor::setNameandPathSensitive(bool ynName, bool ynPath)
{
    survnmfld_->setSensitive( ynName );
    pathfld_->setSensitive( ynPath );
}


IOPar* uiSurveyInfoEditor::getImportPars()
{
    return impiop_;
}


uiSurvInfoProvider* uiSurveyInfoEditor::getSIP()
{
    return lastsip_;
}


// uiSurvInfoProvider
void uiSurvInfoProvider::fillLogPars( IOPar& par ) const
{
    par.setStdCreationEntries();
    par.set( sKey::Type(), usrText() );
}


// uiCopySurveySIP
uiCopySurveySIP::uiCopySurveySIP()
{
}


uiCopySurveySIP::~uiCopySurveySIP()
{}


void uiCopySurveySIP::reset()
{
    deleteAndNullPtr(crspars_);
}


uiDialog* uiCopySurveySIP::dialog( uiParent* p )
{
    survlist_.erase();
    uiSurveySelectDlg* dlg = new uiSurveySelectDlg( p, GetSurveyName(),
	GetBaseDataDir(), true, true );
    dlg->setHelpKey( mODHelpKey(mCopySurveySIPHelpID) );
    return dlg;
}


bool uiCopySurveySIP::getInfo(uiDialog* dlg, TrcKeyZSampling& cs, Coord crd[3])
{
    tdinf_ = Uknown;
    inft_ = false;
    mDynamicCastGet(uiSurveySelectDlg*,seldlg,dlg)
    if ( !seldlg )
	return false;

    const BufferString fname = seldlg->getSurveyPath();
    othersurvey_ = fname;
    PtrMan<SurveyInfo> survinfo = SurveyInfo::readDirectory( fname );
    if ( !survinfo )
	return false;

    cs = survinfo->sampling( false );
    crd[0] = survinfo->transform( cs.hsamp_.start_ );
    crd[1] = survinfo->transform( cs.hsamp_.stop_ );
    crd[2] = survinfo->transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()));

    tdinf_ = survinfo->zIsTime() ? Time
				 : (survinfo->zInFeet() ? DepthFeet : Depth);
    inft_ = survinfo->xyInFeet();

    RefMan<Coords::CoordSystem> crs = survinfo->getCoordSystem();
    IOPar* crspar = new IOPar;
    crs->fillPar( *crspar );
	delete crspars_;
    crspars_ = crspar;

    return true;
}


IOPar* uiCopySurveySIP::getCoordSystemPars() const
{
    if ( !crspars_ )
       return 0;

    return new IOPar( *crspars_ );
}


void uiCopySurveySIP::fillLogPars( IOPar& par ) const
{
    uiSurvInfoProvider::fillLogPars( par );
    par.set( sKey::CrFrom(), othersurvey_ );
}


// uiSurveyFileSIP
uiSurveyFileSIP::uiSurveyFileSIP()
{}


uiSurveyFileSIP::~uiSurveyFileSIP()
{}


const char* uiSurveyFileSIP::usrText() const
{
    return "Read from Survey Setup file";
}


class uiSurveyFileDlg : public uiDialog
{ mODTextTranslationClass(uiSurveyFileDlg)
public:
uiSurveyFileDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Select Survey Setup file"),
			mNoDlgTitle,mTODOHelpKey))
{
    inpfld_ = new uiFileInput( this, uiStrings::sSelect(),
		uiFileInput::Setup().defseldir(GetBaseDataDir())
				    .withexamine(true)
				    .allowallextensions(true) );
    inpfld_->setElemSzPol( uiObject::Wide );
}

uiFileInput* inpfld_;

};


uiDialog* uiSurveyFileSIP::dialog( uiParent* p )
{
    return new uiSurveyFileDlg( p );
}


bool uiSurveyFileSIP::getInfo( uiDialog* dlg, TrcKeyZSampling& cs, Coord crd[3])
{
    tdinf_ = Uknown;
    inft_ = false;
    mDynamicCastGet(uiSurveyFileDlg*,filedlg,dlg)
    if ( !filedlg )
	return false;

    filenm_ = filedlg->inpfld_->fileName();
    PtrMan<SurveyInfo> survinfo = SurveyInfo::readDirectory( filenm_ );
    if ( !survinfo )
	return false;

    cs = survinfo->sampling( false );
    crd[0] = survinfo->transform( cs.hsamp_.start_ );
    crd[1] = survinfo->transform( cs.hsamp_.stop_ );
    crd[2] = survinfo->transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()));

    tdinf_ = survinfo->zIsTime() ? Time
				 : (survinfo->zInFeet() ? DepthFeet : Depth);
    inft_ = survinfo->xyInFeet();
    coordsystem_ = survinfo->getCoordSystem();
    surveynm_ = survinfo->name();

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


void uiSurveyFileSIP::fillLogPars( IOPar& par ) const
{
    uiSurvInfoProvider::fillLogPars( par );
    par.set( sKey::CrFrom(), filenm_ );
}
