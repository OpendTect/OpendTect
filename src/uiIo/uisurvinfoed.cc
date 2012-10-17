/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurvinfoed.h"
#include "uisip.h"

#include "bufstringset.h"
#include "cubesampling.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "mousecursor.h"
#include "ioman.h"
#include "iopar.h"
#include "oddirs.h"
#include "ptrman.h"
#include "statrand.h"
#include "survinfo.h"
#include "systeminfo.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uisurvey.h"


extern "C" const char* GetBaseDataDir();


static ObjectSet<uiSurvInfoProvider>& survInfoProvs()
{
    static ObjectSet<uiSurvInfoProvider>* sips = 0;
    if ( !sips )
	sips = new ObjectSet<uiSurvInfoProvider>;
    return *sips;
}


class uiCopySurveySIP : public uiSurvInfoProvider
{
public:

const char* usrText() const
{ return "Copy from other survey"; }

uiDialog* dialog( uiParent* p )
{
    survlist_.erase();
    uiSurvey::getSurveyList( survlist_ );
    uiSelectFromList::Setup setup( "Surveys", survlist_ );
    setup.dlgtitle( "Select survey" );
    uiSelectFromList* dlg = new uiSelectFromList( p, setup );
    dlg->setHelpID("0.3.6");
    return dlg;
}


bool getInfo( uiDialog* dlg, CubeSampling& cs, Coord crd[3] )
{
    tdinf_ = Uknown;
    inft_ = false;
    mDynamicCastGet(uiSelectFromList*,seldlg,dlg)
    if ( !seldlg ) return false;

    BufferString fname = FilePath( GetBaseDataDir() )
			 .add( seldlg->selFld()->getText() ).fullPath();
    PtrMan<SurveyInfo> survinfo = SurveyInfo::read( fname );
    if ( !survinfo ) return false;

    cs = survinfo->sampling( false );
    crd[0] = survinfo->transform( cs.hrg.start );
    crd[1] = survinfo->transform( cs.hrg.stop );
    crd[2] = survinfo->transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );

    tdinf_ = survinfo->zIsTime() ? Time
				 : (survinfo->zInFeet() ? DepthFeet : Depth);
    inft_ = survinfo->xyInFeet();
    return true;
}

TDInfo tdInfo() const { return tdinf_; }
bool xyInFeet() const { return inft_; }

    TDInfo	tdinf_;
    bool	inft_;
    BufferStringSet survlist_;

};


uiSurveyInfoEditor::uiSurveyInfoEditor( uiParent* p, SurveyInfo& si )
	: uiDialog(p,uiDialog::Setup("Survey setup",
				     "Specify survey parameters","0.3.2")
				     .nrstatusflds(1))
	, rootdir_(GetBaseDataDir())
	, orgdirname_(si_.dirname_.buf())
	, si_(si)
	, survParChanged(this)
	, x0fld_(0)
	, dirnamechanged(false)
	, sipfld_(0)
	, lastsip_(0)
	, impiop_(0)
	, topgrp_( 0 )
{
    static int sipidx mUnusedVar = addInfoProvider( new uiCopySurveySIP );

    orgstorepath_ = si_.datadir_.buf();
    isnew_ = orgdirname_.isEmpty();

    BufferString fulldirpath;
    if ( !isnew_ )
    {
	BufferString storagedir = FilePath(orgstorepath_)
	    			  .add(orgdirname_).fullPath();
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
    else
    {
	orgstorepath_ = rootdir_;
	if ( !File::isWritable(orgstorepath_) )
	{
	    BufferString msg( "Cannot create new survey in\n",orgstorepath_,
			      ".\nDirectory is write protected.");
	    uiMSG().error( msg );
	    return;
	}

	orgdirname_ = newSurvTempDirName();
	BufferString dirnm = FilePath( orgstorepath_ )
	    		    .add( orgdirname_ ).fullPath();
	if ( File::exists(dirnm) && !strncmp(orgdirname_,"_New_",5) )
	    File::remove( dirnm );
	if ( !copySurv(mGetSetupFileName("BasicSurvey"),0,
		       orgstorepath_,orgdirname_) )
	    return;
	File::makeWritable( dirnm, true, true );

	fulldirpath = dirnm;
    }

    IOMan::setSurvey( orgdirname_ );
    SI().setInvalid();
    SurveyInfo::read( fulldirpath );

    topgrp_ = new uiGroup( this, "Top group" );
    survnmfld_ = new uiGenInput( topgrp_, "Survey name",
	    			StringInpSpec(si_.name()) );

    pathfld_ = new uiGenInput( topgrp_, "Location on disk",
	    			StringInpSpec(orgstorepath_) );
    pathfld_->attach( alignedBelow, survnmfld_ );

#ifdef __win__
    pathfld_->setSensitive( false );
#else
    uiButton* pathbut = new uiPushButton( topgrp_, "&Select", false );
    pathbut->attach( rightOf, pathfld_ );
    pathbut->activated.notify( mCB(this,uiSurveyInfoEditor,pathbutPush) );
#endif

    uiLabeledComboBox* lcb = new uiLabeledComboBox( topgrp_,
	    			SurveyInfo::Pol2DNames(), "Survey type" );
    lcb->attach( alignedBelow, pathfld_ ); pol2dfld_ = lcb->box();

    mkSIPFld( lcb->attachObj() );
    if ( sipfld_ )
	topgrp_->setHAlignObj( sipfld_ );
    else
	topgrp_->setHAlignObj( lcb );

    uiSeparator* horsep1 = new uiSeparator( this, "Hor sep 1" );
    horsep1->attach( stretchedBelow, topgrp_, -2 );

    uiLabel* rglbl = new uiLabel( this, "Survey ranges:" );
    rglbl->attach( leftBorder );
    rglbl->attach( ensureBelow, horsep1 );
    mkRangeGrp();
    rangegrp_->attach( alignedBelow, topgrp_ );
    rangegrp_->attach( ensureBelow, horsep1 );

    uiSeparator* horsep2 = new uiSeparator( this, "Hor sep 2" );
    horsep2->attach( stretchedBelow, rangegrp_ );

    uiLabel* crdlbl = new uiLabel( this, "Coordinate settings:" );
    crdlbl->attach( leftBorder );
    crdlbl->attach( ensureBelow, horsep2 );
    coordset = new uiGenInput( this, "", BoolInpSpec(true,"Easy","Advanced") );
    coordset->attach( alignedBelow, rangegrp_ );
    coordset->attach( rightTo, crdlbl );
    coordset->valuechanged.notify( mCB(this,uiSurveyInfoEditor,chgSetMode));

    mkCoordGrp();
    crdgrp_->attach( alignedBelow, rangegrp_ );
    crdgrp_->attach( ensureBelow, coordset );

    mkTransfGrp();
    trgrp_->attach( alignedBelow, rangegrp_ );
    trgrp_->attach( ensureBelow, coordset );

    uiPushButton* applybut = new uiPushButton( this, "&Apply", true ); 
    applybut->activated.notify( mCB(this,uiSurveyInfoEditor,appButPushed) );
    applybut->attach( alignedBelow, crdgrp_ );

    xyinftfld_ = new uiCheckBox( this, "Coordinates are in feet" );
    xyinftfld_->attach( rightTo, applybut );
    xyinftfld_->attach( rightBorder );
    xyinftfld_->activated.notify( mCB(this,uiSurveyInfoEditor,updZUnit) );
    zinftfld_ = new uiCheckBox( this, "Display depths in feet" );
    zinftfld_->attach( leftTo, applybut );
    zinftfld_->attach( leftBorder );

    postFinalise().notify( mCB(this,uiSurveyInfoEditor,doFinalise) );
}


uiSurveyInfoEditor::~uiSurveyInfoEditor()
{
    delete impiop_;
}


void uiSurveyInfoEditor::mkSIPFld( uiObject* att )
{
    const int nrprovs = survInfoProvs().size();
    if ( nrprovs < 1 ) return;

    int maxlen = 0;
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	int len = strlen( survInfoProvs()[idx]->usrText() );
	if ( len > maxlen ) maxlen = len;
    }

    uiLabeledComboBox* lcb = new uiLabeledComboBox( topgrp_,
					    "Ranges/coordinate settings" );
    lcb->attach( alignedBelow, att );
    sipfld_ = lcb->box();
    sipfld_->addItem( "Enter below" );
    sipfld_->selectionChanged.notify( mCB(this,uiSurveyInfoEditor,sipCB) );
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	BufferString txt( survInfoProvs()[idx]->usrText() );
	txt += " ...";
	sipfld_->addItem( txt );
    }
    sipfld_->setCurrentItem( 0 );
    sipfld_->setPrefWidthInChar( maxlen + 1 );
}


void uiSurveyInfoEditor::mkRangeGrp()
{
    rangegrp_ = new uiGroup( this, "Survey ranges" );
    inlfld_ = new uiGenInput( rangegrp_, "In-line range",
			     IntInpIntervalSpec(true).setName("Inl Start",0)
			     			     .setName("Inl Stop",1)
	   					     .setName("Inl step",2) );
    inlfld_->valuechanged.notify( mCB(this,uiSurveyInfoEditor,rangeChg) );
    crlfld_ = new uiGenInput( rangegrp_, "Cross-line range",
			     IntInpIntervalSpec(true).setName("Crl Start",0)
				 		     .setName("Crl Stop",1) 
						     .setName("Crl step",2) );
    crlfld_->valuechanged.notify( mCB(this,uiSurveyInfoEditor,rangeChg) );
    zfld_ = new uiGenInput( rangegrp_, "Z range", 
	    	 	   DoubleInpIntervalSpec(true).setName("Z Start",0)
	   					      .setName("Z Stop",1) 
						      .setName("Z step",2) );
    zfld_->valuechanged.notify( mCB(this,uiSurveyInfoEditor,rangeChg) );
    crlfld_->attach( alignedBelow, inlfld_ );
    zfld_->attach( alignedBelow, crlfld_ );

    static const char* zunitstrs[] = { "msec", "meter", "feet", 0 };
    zunitfld_ = new uiComboBox( rangegrp_, zunitstrs, "Z unit" );
    zunitfld_->attach( rightOf, zfld_ );
    zunitfld_->setHSzPol( uiObject::Small );
    zunitfld_->selectionChanged.notify( mCB(this,uiSurveyInfoEditor,updZUnit) );

    rangegrp_->setHAlignObj( inlfld_ );
}


void uiSurveyInfoEditor::mkCoordGrp()
{
    crdgrp_ = new uiGroup( this, "Coordinate settings" );
    PositionInpSpec::Setup psetup;
    ic0fld_ = new uiGenInput( crdgrp_, "First In-line/Cross-line", 
		     PositionInpSpec(psetup).setName("Inl Position1",0)
	   				    .setName("Crl Position1",1) ); 
    ic0fld_->valuechanging.notify( mCB(this,uiSurveyInfoEditor,setInl1Fld) );
    ic1fld_ = new uiGenInput( crdgrp_, "Another position on above In-line",
		     PositionInpSpec(psetup).setName("Inl Position2",0)
	   				    .setName("Crl Position2",1) ); 
    ic2fld_ = new uiGenInput( crdgrp_, "Position not on above In-line",
		      PositionInpSpec(psetup).setName("Inl Position3",0)
	   				     .setName("Crl Position3",1) ); 
    psetup.wantcoords_ = true;
    xy0fld_ = new uiGenInput( crdgrp_, "= (X,Y)", 
	    			PositionInpSpec(psetup).setName("X1",0)
	   					       .setName("Y1",1) );
    xy1fld_ = new uiGenInput( crdgrp_, "= (X,Y)",
	    			PositionInpSpec(psetup).setName("X2",0)
	   					       .setName("Y2",1) );
    xy2fld_ = new uiGenInput( crdgrp_, "= (X,Y)",
	    			PositionInpSpec(psetup).setName("X3",0)
	   					       .setName("Y3",1) );
    ic1fld_->attach( alignedBelow, ic0fld_ );
    ic2fld_->attach( alignedBelow, ic1fld_ );
    xy0fld_->attach( rightOf, ic0fld_ );
    xy1fld_->attach( rightOf, ic1fld_ );
    xy2fld_->attach( rightOf, ic2fld_ );

    crdgrp_->setHAlignObj( ic0fld_ );
}


void uiSurveyInfoEditor::mkTransfGrp()
{
    trgrp_ = new uiGroup( this, "I/C to X/Y transformation" );
    x0fld_ = new uiGenInput ( trgrp_, "X = ", DoubleInpSpec().setName("X") );
    x0fld_->setElemSzPol( uiObject::Small );
    xinlfld_ = new uiGenInput ( trgrp_, "+ in-line *",
	   			       DoubleInpSpec().setName("Inl") );
    xinlfld_->setElemSzPol( uiObject::Small );
    xcrlfld_ = new uiGenInput ( trgrp_, "+ cross-line *",
	   			      DoubleInpSpec().setName("Crl") );
    xcrlfld_->setElemSzPol( uiObject::Small );
    y0fld_ = new uiGenInput ( trgrp_, "Y = ", DoubleInpSpec().setName("Y"));
    y0fld_->setElemSzPol( uiObject::Small );
    yinlfld_ = new uiGenInput ( trgrp_, "+ in-line *",
	    			      DoubleInpSpec() .setName("Inl"));
    yinlfld_->setElemSzPol( uiObject::Small );
    ycrlfld_ = new uiGenInput ( trgrp_, "+ cross-line *",
	    			      DoubleInpSpec() .setName("Crl"));
    ycrlfld_->setElemSzPol( uiObject::Small );
    overrulefld_ = new uiCheckBox( trgrp_, "Overrule easy settings" );
    overrulefld_->setChecked( false );
    xinlfld_->attach( rightOf, x0fld_ );
    xcrlfld_->attach( rightOf, xinlfld_ );
    y0fld_->attach( alignedBelow, x0fld_ );
    yinlfld_->attach( rightOf, y0fld_ );
    ycrlfld_->attach( rightOf, yinlfld_ );
    overrulefld_->attach( alignedBelow, ycrlfld_ );
    trgrp_->setHAlignObj( xinlfld_ );
}


static void setZValFld( uiGenInput* zfld, int nr, float val, float fac )
{
    if ( mIsUdf(val) )
	{ zfld->setText( "", nr ); return; }

    val *= fac; int ival = mNINT32(val); float fival = ival;
    if ( mIsEqual(val,fival,0.01) )
	zfld->setValue( ival, nr );
    else
	zfld->setValue( val, nr );
}


void uiSurveyInfoEditor::setValues()
{
    const CubeSampling& cs = si_.sampling( false );
    const HorSampling& hs = cs.hrg;
    StepInterval<int> inlrg( hs.start.inl, hs.stop.inl, hs.step.inl );
    StepInterval<int> crlrg( hs.start.crl, hs.stop.crl, hs.step.crl );
    inlfld_->setValue( inlrg );
    crlfld_->setValue( crlrg );

    const StepInterval<float>& zrg = si_.zRange( false );
    const float zfac = si_.zDomain().userFactor();
    setZValFld( zfld_, 0, zrg.start, zfac );
    setZValFld( zfld_, 1, zrg.stop, zfac );
    setZValFld( zfld_, 2, zrg.step, zfac );

    zunitfld_->setCurrentItem( si_.zIsTime()  ? 0 : (si_.zInMeter() ? 1 : 2) );

    x0fld_->setValue( si_.b2c_.getTransform(true).a );
    xinlfld_->setValue( si_.b2c_.getTransform(true).b );
    xcrlfld_->setValue( si_.b2c_.getTransform(true).c );
    y0fld_->setValue( si_.b2c_.getTransform(false).a );
    yinlfld_->setValue( si_.b2c_.getTransform(false).b );
    ycrlfld_->setValue( si_.b2c_.getTransform(false).c );

    Coord c[3]; BinID b[2]; int xline;
    si_.get3Pts( c, b, xline );
    if ( b[0].inl )
    {
	ic0fld_->setValue( b[0] );
	ic1fld_->setValues( b[0].inl, xline );
	ic2fld_->setValue( b[1] );
	if ( !c[0].x && !c[0].y && !c[1].x && !c[1].y && !c[2].x && !c[2].y)
	{
	    c[0] = si_.transform( b[0] );
	    c[1] = si_.transform( b[1] );
	    c[2] = si_.transform( BinID(b[0].inl,xline) );
	}
	xy0fld_->setValue( c[0] );
	xy1fld_->setValue( c[2] );
	xy2fld_->setValue( c[1] );
    }

    xyinftfld_->setChecked( si_.xyInFeet() );
    zinftfld_->setChecked( si_.depthsInFeetByDefault() );
    updZUnit( 0 );
}


int uiSurveyInfoEditor::addInfoProvider( uiSurvInfoProvider* p )
{
    if ( p ) survInfoProvs() += p;
    return survInfoProvs().size();
}


const char* uiSurveyInfoEditor::newSurvTempDirName()
{
    static BufferString nm;
    nm = "_New_Survey_";
    const char* usr = GetSoftwareUser();
    if ( usr )
	{ nm += usr; nm += "_"; }
    nm += GetPID();
    Stats::randGen().init();
    nm += Stats::randGen().getIndex(1000000);
    return nm.buf();
}


bool uiSurveyInfoEditor::copySurv( const char* inpath, const char* indirnm,
				   const char* outpath, const char* outdirnm )
{
    const BufferString fnmin = FilePath(inpath).add(indirnm).fullPath();
    const BufferString fnmout = FilePath(outpath).add(outdirnm).fullPath();
    if ( File::exists(fnmout) )
    {
	BufferString msg( "Cannot copy " ); msg += fnmin;
	msg += " to "; msg += fnmout;
	msg += "\nbecause target directory exists";
	uiMSG().error( msg );
	return false;
    }
    MouseCursorManager::setOverride( MouseCursor::Wait );
    File::copy( fnmin, fnmout );
    MouseCursorManager::restoreOverride();
    if ( !File::exists(fnmout) )
    {
	BufferString msg( "Copy " ); msg += fnmin;
	msg += " to "; msg += fnmout; msg += " failed\n"
	    "See startup window for details";
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
	BufferString msg( "Cannot rename " ); msg += fnmin;
	msg += " to "; msg += fnmout;
	msg += "\nbecause target directory exists";
	uiMSG().error( msg );
	return false;
    }
    File::rename( fnmin, fnmout );
    if ( !File::exists(fnmout) )
    {
	BufferString msg( "Rename " ); msg += fnmin;
	msg += " to "; msg += fnmout; msg += " failed\n"
	    "See startup window for details";
	uiMSG().error( msg );
	return false;
    }

    return true;
}


#define mUseAdvanced() (overrulefld_->isChecked() && !coordset->getBoolValue())

void uiSurveyInfoEditor::appButPushed( CallBacker* )
{
    doApply();
}


bool uiSurveyInfoEditor::doApply()
{
    if ( !setSurvName() || !setRanges() )
	return false;

    const bool xyinft = xyinftfld_->isChecked();
    si_.setXYInFeet( xyinft );
    const bool zdepthft = zunitfld_->currentItem() == 2;
    const_cast<IOPar&>(si_.pars()).setYN( SurveyInfo::sKeyDpthInFt(),
	    xyinft || zdepthft || zinftfld_->isChecked() );

    if ( !mUseAdvanced() )
    {
	if ( !setCoords() ) return false;

	x0fld_->setValue( si_.b2c_.getTransform(true).a );
	xinlfld_->setValue( si_.b2c_.getTransform(true).b );
	xcrlfld_->setValue( si_.b2c_.getTransform(true).c );
	y0fld_->setValue( si_.b2c_.getTransform(false).a );
	yinlfld_->setValue( si_.b2c_.getTransform(false).b );
	ycrlfld_->setValue( si_.b2c_.getTransform(false).c );
	overrulefld_->setChecked( false );
    }
    else if ( !setRelation() )
	return false;

    survParChanged.trigger();
    return true;
}


void uiSurveyInfoEditor::doFinalise( CallBacker* )
{
    pathfld_->setText( orgstorepath_ );
    pathfld_->setReadOnly( true );
    updStatusBar( orgstorepath_ );

    pol2dfld_->setCurrentItem( (int)si_.survDataType() );
    
    if ( si_.sampling(false).hrg.totalNr() )
	setValues();

    chgSetMode(0);
    ic1fld_->setReadOnly( true, 0 );
    updZUnit( 0 );
}


bool uiSurveyInfoEditor::rejectOK( CallBacker* )
{
    if ( isnew_ && !strncmp(orgdirname_,"_New_",5) )
    {
	const BufferString dirnm = FilePath(orgstorepath_)
	    			   .add(orgdirname_).fullPath();
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
	uiMSG().error( "Please specify a valid survey name" );
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
    const BufferString newdir( FilePath(newstorepath).add(newdirnm).fullPath());
    const bool storepathchanged = orgstorepath_ != newstorepath;
    dirnamechanged = orgdirname_ != newdirnm;

    if ( !isnew_ )
    {
	if ( (dirnamechanged || storepathchanged)
	  && File::exists(newdir) )
	{
	    uiMSG().error( "The new target directory exists.\n"
		    	   "Please enter another survey name or location." );
	    return false;
	}

	if ( storepathchanged )
	{
	    if ( !uiMSG().askGoOn("Copy your survey to another location?") )
		return false;
	    else if ( !copySurv(orgstorepath_,orgdirname_,
				newstorepath,newdirnm) )
		return false;
	    else if ( !uiMSG().askGoOn("Keep the survey at the old location?") )
		File::remove( olddir );
	}
	else if ( dirnamechanged )
	{
	    if ( !renameSurv(orgstorepath_,orgdirname_,newdirnm) )
		return false;
	}
    }
    else
    {
	if ( File::exists(newdir) )
	{
	    uiMSG().error( "The chosen target directory exists.\n"
		    	   "Please enter another name or location." );
	    return false;
	}

	if ( newstorepath != orgstorepath_ )
	{
	    if ( !copySurv(orgstorepath_,orgdirname_,newstorepath,newdirnm) )
		return false;
	    File::remove( olddir );
	}
	else if ( !renameSurv(newstorepath,orgdirname_,newdirnm) )
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
	    BufferString msg( "Cannot create link from \n" );
	    msg += newdir; msg += " to \n"; msg += linkpos;
	    uiMSG().error( msg ); 
	    return false;
	}
    }

    si_.dirname_ = newdirnm;
    si_.setSurvDataType( (SurveyInfo::Pol2D)pol2dfld_->currentItem() );
    if ( mUseAdvanced() )
	si_.get3Pts( si_.set3coords_, si_.set3binids_, si_.set3binids_[2].crl );

    if ( !si_.write(rootdir_) )
    {
        uiMSG().error( "Failed to write survey info.\nNo changes committed." );
	return false;
    }
    
    return true;
}


const char* uiSurveyInfoEditor::dirName() const
{
    static BufferString ret; ret = survnmfld_->text();
    cleanupString( ret.buf(), false, false, true );
    return ret.buf();
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSurveyInfoEditor::setRanges()
{
    StepInterval<int> irg( inlfld_->getIStepInterval() );
    StepInterval<int> crg( crlfld_->getIStepInterval() );
    CubeSampling cs( si_.sampling(false) );
    HorSampling& hs = cs.hrg;
    hs.start.inl = irg.start; hs.start.crl = crg.start;
    hs.stop.inl = irg.atIndex( irg.getIndex(irg.stop) );
    hs.stop.crl = crg.atIndex( crg.getIndex(crg.stop) );
    hs.step.inl = irg.step;   hs.step.crl = crg.step;
    if ( hs.step.inl < 1 ) hs.step.inl = 1;
    if ( hs.step.crl < 1 ) hs.step.crl = 1;

    const int curzunititem = zunitfld_->currentItem();
    si_.setZUnit( curzunititem == 0, curzunititem == 2 );
    cs.zrg = zfld_->getFStepInterval();
    if ( mIsUdf(cs.zrg.start) || mIsUdf(cs.zrg.stop) || mIsUdf(cs.zrg.step) )
	mErrRet("Please enter the Z Range")
    const float zfac = 1.f / si_.zDomain().userFactor();
    if ( !mIsEqual(zfac,1,0.0001) )
	{ cs.zrg.start *= zfac; cs.zrg.stop *= zfac; cs.zrg.step *= zfac; }
    if ( mIsZero(cs.zrg.step,1e-8) )
	cs.zrg.step = si_.zIsTime() ? 0.004f : 1;
    cs.normalise();
    if ( !hs.totalNr() )
	mErrRet("Please specify inline/crossline ranges")
    if ( cs.zrg.nrSteps() == 0 )
	mErrRet("Please specify a valid Z range")

    si_.setRange( cs, false );
    return true;
}


bool uiSurveyInfoEditor::setCoords()
{
    BinID b[2]; Coord c[3]; int xline;
    b[0] = ic0fld_->getBinID();
    b[1] = ic2fld_->getBinID();
    xline = ic1fld_->getBinID().crl;
    c[0] = xy0fld_->getCoord();
    c[1] = xy2fld_->getCoord();
    c[2] = xy1fld_->getCoord();
  
    const char* msg = si_.set3Pts( c, b, xline );
    if ( msg ) { uiMSG().error( msg ); return false; }
    else if ( mUseAdvanced() )
	si_.gen3Pts();

    return true;
}


bool uiSurveyInfoEditor::setRelation()
{
    RCol2Coord::RCTransform xtr, ytr;
    xtr.a = x0fld_->getdValue();   ytr.a = y0fld_->getdValue();
    xtr.b = xinlfld_->getdValue(); ytr.b = yinlfld_->getdValue();
    xtr.c = xcrlfld_->getdValue(); ytr.c = ycrlfld_->getdValue();
    if ( !xtr.valid(ytr) )
    {
        uiMSG().error( "The transformation is not valid." );
        return false;
    }

    si_.b2c_.setTransforms( xtr, ytr );
    return true;
}


void uiSurveyInfoEditor::sipCB( CallBacker* cb )
{
    const int sipidx = sipfld_ ? sipfld_->currentItem() : 0;
    if ( sipidx < 1 ) return;
    sipfld_->setCurrentItem( 0 );
    delete impiop_; impiop_ = 0; lastsip_ = 0;

    const int curzunititem = zunitfld_->currentItem();
    si_.setZUnit( curzunititem == 0, curzunititem == 2 );

    uiSurvInfoProvider* sip = survInfoProvs()[sipidx-1];
    PtrMan<uiDialog> dlg = sip->dialog( this );
    if ( !dlg || !dlg->go() ) return;

    CubeSampling cs; Coord crd[3];
    if ( !sip->getInfo(dlg,cs,crd) )
	return;

    if ( sip->tdInfo() != uiSurvInfoProvider::Uknown )
	si_.setZUnit( sip->tdInfo() == uiSurvInfoProvider::Time,
		      sip->tdInfo() == uiSurvInfoProvider::DepthFeet );
    si_.setXYInFeet( sip->xyInFeet() );

    const bool havez = !mIsUdf(cs.zrg.start);
    if ( !havez )
	cs.zrg = si_.zRange(false);

    si_.setRange(cs,false);
    BinID bid[2];
    bid[0].inl = cs.hrg.start.inl; bid[0].crl = cs.hrg.start.crl;
    bid[1].inl = cs.hrg.stop.inl; bid[1].crl = cs.hrg.stop.crl;
    si_.set3Pts( crd, bid, cs.hrg.stop.crl );
    setValues();
    if ( !havez ) zfld_->clear();
    const bool xyinft = xyinftfld_->isChecked();
    si_.setXYInFeet( xyinft );
    const_cast<IOPar&>(si_.pars()).setYN( SurveyInfo::sKeyDpthInFt(),
	    		xyinft || zinftfld_->isChecked() );

    si_.setWSProjName( SI().getWSProjName() );
    si_.setWSPwd( SI().getWSPwd() );

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
	    uiMSG().error( "Directory is not writable" );
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
    toStatusBar( msg );
}


void uiSurveyInfoEditor::chgSetMode( CallBacker* )
{
    crdgrp_->display( coordset->getBoolValue() );
    trgrp_->display( !coordset->getBoolValue() );
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
	if ( mIsUdf(irg.step) || !irg.step ) return;

	irg.stop = irg.atIndex( irg.getIndex(irg.stop) );
	inlfld_->setValue( irg );
    }
    else if ( cb == crlfld_ )
    {
	StepInterval<int> crg = crlfld_->getIStepInterval();
	if ( mIsUdf(crg.step) || !crg.step ) return;

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
}


void uiSurveyInfoEditor::updZUnit( CallBacker* )
{
    const bool xyinft = xyinftfld_->isChecked();
    zinftfld_->setSensitive( !xyinft );
    zinftfld_->display( zunitfld_->currentItem() == 0 );

    if ( xyinft )
    {
	NotifyStopper ns( zinftfld_->activated );
	zinftfld_->setChecked( true );
    }
}
