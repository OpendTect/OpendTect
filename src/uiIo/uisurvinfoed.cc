/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisurvinfoed.cc,v 1.109 2009-04-20 07:13:27 cvsnanne Exp $";

#include "uisurvinfoed.h"
#include "uisip.h"

#include "bufstringset.h"
#include "cubesampling.h"
#include "errh.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h" // for GetFreeMBOnDiskMsg
#include "mousecursor.h"
#include "oddirs.h"
#include "ptrman.h"
#include "survinfo.h"
#include "statrand.h"
#include "iopar.h"

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
	, rootdir(GetBaseDataDir())
	, orgdirname(si_.dirname.buf())
	, si_(si)
	, survparchanged(this)
	, x0fld(0)
	, dirnamechanged(false)
	, sipfld(0)
	, lastsip_(0)
	, impiop_(0)
{
    static int sipidx = addInfoProvider( new uiCopySurveySIP );

    orgstorepath = si_.datadir.buf();
    isnew = orgdirname.isEmpty();

    BufferString fulldirpath;
    if ( !isnew )
    {
	BufferString storagedir = FilePath(orgstorepath)
	    			  .add(orgdirname).fullPath();
	int linkcount = 0;
	while ( linkcount++ < 20 && File_isLink(storagedir) )
	{
	    BufferString newstoragedir = File_linkTarget(storagedir);
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
	    orgstorepath = fp.pathOnly();
	    orgdirname = fp.fileName();
	}

	fulldirpath = storagedir;
    }
    else
    {
	orgstorepath = rootdir;
	orgdirname = newSurvTempDirName();
	BufferString dirnm = FilePath( orgstorepath )
	    		    .add( orgdirname ).fullPath();
	if ( File_exists(dirnm) )
	    File_remove( dirnm, mFile_Recursive );
	if ( !copySurv(mGetSetupFileName("BasicSurvey"),0,
		       orgstorepath,orgdirname) )
	    return;
	File_makeWritable( dirnm, mFile_Recursive, mC_True );

	fulldirpath = dirnm;
    }

    IOMan::setSurvey( orgdirname );
    SI().setInvalid();
    SurveyInfo::read( fulldirpath );

    survnmfld = new uiGenInput( this, "Survey name",
	    			StringInpSpec(si_.name()) );

    pathfld = new uiGenInput( this, "Location on disk",
	    			StringInpSpec(orgstorepath) );
    pathfld->attach( alignedBelow, survnmfld );

#ifdef __win__
    pathfld->setSensitive( false );
#else
    uiButton* pathbut = new uiPushButton( this, "&Select", false );
    pathbut->attach( rightOf, pathfld );
    pathbut->activated.notify( mCB(this,uiSurveyInfoEditor,pathbutPush) );
#endif

    pol2dfld = new uiLabeledComboBox( this, SurveyInfo::Pol2DNames(),
	    			      "Survey type" );
    pol2dfld->attach( alignedBelow, pathfld );

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->attach( stretchedBelow, pol2dfld, -2 );

    uiObject* sipobj = mkSIPFld( horsep1 );

    uiLabel* rglbl = new uiLabel( this, "Survey ranges:" );
    rglbl->attach( leftBorder );
    rglbl->attach( ensureBelow, horsep1 );
    uiGroup* rangegrp = mkRangeGrp();
    if ( sipobj )
	rangegrp->attach( alignedBelow, sipobj ); 
    else
    {
	rangegrp->attach( alignedBelow, pathfld ); 
	rangegrp->attach( ensureBelow, rglbl ); 
    }

    uiSeparator* horsep2 = new uiSeparator( this );
    horsep2->attach( stretchedBelow, rangegrp );

    uiLabel* crdlbl = new uiLabel( this, "Coordinate settings:" );
    crdlbl->attach( leftBorder );
    crdlbl->attach( ensureBelow, horsep2 );
    coordset = new uiGenInput( this, "", BoolInpSpec(true,"Easy","Advanced") );
    coordset->attach( alignedBelow, rangegrp );
    coordset->attach( rightTo, crdlbl );
    coordset->valuechanged.notify( mCB(this,uiSurveyInfoEditor,chgSetMode));

    mkCoordGrp();
    crdgrp->attach( alignedBelow, rangegrp );
    crdgrp->attach( ensureBelow, coordset );

    mkTransfGrp();
    trgrp->attach( alignedBelow, rangegrp );
    trgrp->attach( ensureBelow, coordset );

    applybut = new uiPushButton( this, "&Apply", true ); 
    applybut->activated.notify( mCB(this,uiSurveyInfoEditor,appButPushed) );
    applybut->attach( alignedBelow, crdgrp );
    xyinftfld = new uiCheckBox( this, "Coordinates are in feet" );
    xyinftfld->attach( rightTo, applybut );
    xyinftfld->attach( rightBorder );

    finaliseDone.notify( mCB(this,uiSurveyInfoEditor,doFinalise) );
}


uiSurveyInfoEditor::~uiSurveyInfoEditor()
{
    delete impiop_;
}


uiObject* uiSurveyInfoEditor::mkSIPFld( uiObject* horsep )
{
    const int nrprovs = survInfoProvs().size();
    if ( nrprovs < 1 ) return 0;

    CallBack sipcb( mCB(this,uiSurveyInfoEditor,sipbutPush) );
    int maxlen = 0;
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	int len = strlen( survInfoProvs()[idx]->usrText() );
	if ( len > maxlen ) maxlen = len;
    }

    uiObject* ret = 0;
    if ( nrprovs > 2 )
    {
	ret = sipfld = new uiComboBox( this, "SIPs" );
	sipfld->attach( alignedBelow, pathfld );
	sipfld->attach( ensureBelow, horsep );
	for ( int idx=0; idx<nrprovs; idx++ )
	{
	    BufferString txt( survInfoProvs()[idx]->usrText() );
	    txt += " ->>";
	    sipfld->addItem( txt );
	}
	sipfld->setCurrentItem( nrprovs-2 ); // last one before copy
	sipfld->setPrefWidthInChar( maxlen + 10 );
	uiPushButton* sipbut = new uiPushButton( this, "&Go", sipcb, false);
	sipbut->attach( rightOf, sipfld );
    }
    else
    {
	uiPushButton* prevbut = 0;
	for ( int idx=0; idx<nrprovs; idx++ )
	{
	    uiPushButton* newpb = new uiPushButton( this,
		    survInfoProvs()[idx]->usrText(), sipcb, false );
	    sipbuts += newpb;
	    if ( prevbut )
		newpb->attach( rightOf, prevbut );
	    else
	    {
		newpb->attach( alignedBelow, pathfld );
		newpb->attach( ensureBelow, horsep );
		ret = newpb;
	    }
	    newpb->setPrefWidthInChar( maxlen + 6 );
	    prevbut = newpb;
	}
    }

    return ret;
}


uiGroup* uiSurveyInfoEditor::mkRangeGrp()
{
    uiGroup* rangegrp = new uiGroup( this, "Survey ranges" );
    inlfld = new uiGenInput( rangegrp, "In-line range",
			     IntInpIntervalSpec(true).setName("Inl Start",0)
			     			     .setName("Inl Stop",1)
	   					     .setName("Inl step",2) );
    crlfld = new uiGenInput( rangegrp, "Cross-line range",
			     IntInpIntervalSpec(true).setName("Crl Start",0)
				 		     .setName("Crl Stop",1) 
						     .setName("Crl step",2) );
    zfld = new uiGenInput( rangegrp, "Z range", 
	    	 	   DoubleInpIntervalSpec(true).setName("Z Start",0)
	   					      .setName("Z Stop",1) 
						      .setName("Z step",2) );
    crlfld->attach( alignedBelow, inlfld );
    zfld->attach( alignedBelow, crlfld );

    static const char* zunitstrs[] = { "msec", "meter", "feet", 0 };
    zunitfld = new uiLabeledComboBox( rangegrp, zunitstrs, "Z unit" );
    zunitfld->attach( alignedBelow, zfld );

    rangegrp->setHAlignObj( inlfld );
    return rangegrp;
}


void uiSurveyInfoEditor::mkCoordGrp()
{
    crdgrp = new uiGroup( this, "Coordinate settings" );
    PositionInpSpec::Setup psetup;
    ic0fld = new uiGenInput( crdgrp, "First In-line/Cross-line", 
		     PositionInpSpec(psetup).setName("Inl Position1",0)
	   				    .setName("Crl Position1",1) ); 
    ic0fld->valuechanging.notify( mCB(this,uiSurveyInfoEditor,setInl1Fld) );
    ic1fld = new uiGenInput( crdgrp, "Another position on above In-line",
		     PositionInpSpec(psetup).setName("Inl Position2",0)
	   				    .setName("Crl Position2",1) ); 
    ic2fld = new uiGenInput( crdgrp, "Position not on above In-line",
		      PositionInpSpec(psetup).setName("Inl Position3",0)
	   				     .setName("Crl Position3",1) ); 
    psetup.wantcoords_ = true;
    xy0fld = new uiGenInput( crdgrp, "= (X,Y)", 
	    			PositionInpSpec(psetup).setName("X1",0)
	   					       .setName("Y1",1) );
    xy1fld = new uiGenInput( crdgrp, "= (X,Y)",
	    			PositionInpSpec(psetup).setName("X2",0)
	   					       .setName("Y2",1) );
    xy2fld = new uiGenInput( crdgrp, "= (X,Y)",
	    			PositionInpSpec(psetup).setName("X3",0)
	   					       .setName("Y3",1) );
    ic1fld->attach( alignedBelow, ic0fld );
    ic2fld->attach( alignedBelow, ic1fld );
    xy0fld->attach( rightOf, ic0fld );
    xy1fld->attach( rightOf, ic1fld );
    xy2fld->attach( rightOf, ic2fld );

    crdgrp->setHAlignObj( ic0fld );
}


void uiSurveyInfoEditor::mkTransfGrp()
{
    trgrp = new uiGroup( this, "I/C to X/Y transformation" );
    x0fld = new uiGenInput ( trgrp, "X = ", DoubleInpSpec().setName("X") );
    x0fld->setElemSzPol( uiObject::Small );
    xinlfld = new uiGenInput ( trgrp, "+ in-line *",
	   			       DoubleInpSpec().setName("Inl") );
    xinlfld->setElemSzPol( uiObject::Small );
    xcrlfld = new uiGenInput ( trgrp, "+ cross-line *",
	   			      DoubleInpSpec().setName("Crl") );
    xcrlfld->setElemSzPol( uiObject::Small );
    y0fld = new uiGenInput ( trgrp, "Y = ", DoubleInpSpec().setName("Y"));
    y0fld->setElemSzPol( uiObject::Small );
    yinlfld = new uiGenInput ( trgrp, "+ in-line *",
	    			      DoubleInpSpec() .setName("Inl"));
    yinlfld->setElemSzPol( uiObject::Small );
    ycrlfld = new uiGenInput ( trgrp, "+ cross-line *",
	    			      DoubleInpSpec() .setName("Crl"));
    ycrlfld->setElemSzPol( uiObject::Small );
    overrulefld = new uiCheckBox( trgrp, "Overrule easy settings" );
    overrulefld->setChecked( false );
    xinlfld->attach( rightOf, x0fld );
    xcrlfld->attach( rightOf, xinlfld );
    y0fld->attach( alignedBelow, x0fld );
    yinlfld->attach( rightOf, y0fld );
    ycrlfld->attach( rightOf, yinlfld );
    overrulefld->attach( alignedBelow, ycrlfld );
    trgrp->setHAlignObj( xinlfld );
}


static void setZValFld( uiGenInput* zfld, int nr, float val, float fac )
{
    if ( mIsUdf(val) )
	{ zfld->setText( "", nr ); return; }

    val *= fac; int ival = mNINT(val); float fival = ival;
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
    inlfld->setValue( inlrg );
    crlfld->setValue( crlrg );

    const StepInterval<float>& zrg = si_.zRange( false );
    const float zfac = si_.zFactor();
    setZValFld( zfld, 0, zrg.start, zfac );
    setZValFld( zfld, 1, zrg.stop, zfac );
    setZValFld( zfld, 2, zrg.step, zfac );

    zunitfld->box()->setCurrentItem( si_.zIsTime()  ? 0
	    			  : (si_.zInMeter() ? 1 : 2) );

    x0fld->setValue( si_.b2c_.getTransform(true).a );
    xinlfld->setValue( si_.b2c_.getTransform(true).b );
    xcrlfld->setValue( si_.b2c_.getTransform(true).c );
    y0fld->setValue( si_.b2c_.getTransform(false).a );
    yinlfld->setValue( si_.b2c_.getTransform(false).b );
    ycrlfld->setValue( si_.b2c_.getTransform(false).c );

    Coord c[3]; BinID b[2]; int xline;
    si_.get3Pts( c, b, xline );
    if ( b[0].inl )
    {
	ic0fld->setValue( b[0] );
	ic1fld->setValues( b[0].inl, xline );
	ic2fld->setValue( b[1] );
	if ( !c[0].x && !c[0].y && !c[1].x && !c[1].y && !c[2].x && !c[2].y)
	{
	    c[0] = si_.transform( b[0] );
	    c[1] = si_.transform( b[1] );
	    c[2] = si_.transform( BinID(b[0].inl,xline) );
	}
	xy0fld->setValue( c[0] );
	xy1fld->setValue( c[2] );
	xy2fld->setValue( c[1] );
    }

    xyinftfld->setChecked( si_.xyInFeet() );
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
    Stats::RandGen::init();
    nm += Stats::RandGen::getIndex(1000000);
    return nm.buf();
}


bool uiSurveyInfoEditor::copySurv( const char* inpath, const char* indirnm,
				   const char* outpath, const char* outdirnm )
{
    const BufferString fnmin = FilePath(inpath).add(indirnm).fullPath();
    const BufferString fnmout = FilePath(outpath).add(outdirnm).fullPath();
    if ( File_exists(fnmout) )
    {
	BufferString msg( "Cannot copy " ); msg += fnmin;
	msg += " to "; msg += fnmout;
	msg += "\nbecause target directory exists";
	uiMSG().error( msg );
	return false;
    }
    MouseCursorManager::setOverride( MouseCursor::Wait );
    File_copy( fnmin, fnmout, mFile_Recursive );
    MouseCursorManager::restoreOverride();
    if ( !File_exists(fnmout) )
    {
	BufferString msg( "Copy " ); msg += fnmin;
	msg += " to "; msg += fnmout; msg += " failed\n"
	    "See starup window for details";
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
    if ( File_exists(fnmout) )
    {
	BufferString msg( "Cannot rename " ); msg += fnmin;
	msg += " to "; msg += fnmout;
	msg += "\nbecause target directory exists";
	uiMSG().error( msg );
	return false;
    }
    File_rename( fnmin, fnmout );
    if ( !File_exists(fnmout) )
    {
	BufferString msg( "Rename " ); msg += fnmin;
	msg += " to "; msg += fnmout; msg += " failed\n"
	    "See starup window for details";
	uiMSG().error( msg );
	return false;
    }

    return true;
}


#define mUseAdvanced() (overrulefld->isChecked() && !coordset->getBoolValue())

void uiSurveyInfoEditor::appButPushed( CallBacker* )
{
    doApply();
}


bool uiSurveyInfoEditor::doApply()
{
    if ( !setSurvName() || !setRanges() )
	return false;

    if ( !mUseAdvanced() )
    {
	if ( !setCoords() ) return false;

	x0fld->setValue( si_.b2c_.getTransform(true).a );
	xinlfld->setValue( si_.b2c_.getTransform(true).b );
	xcrlfld->setValue( si_.b2c_.getTransform(true).c );
	y0fld->setValue( si_.b2c_.getTransform(false).a );
	yinlfld->setValue( si_.b2c_.getTransform(false).b );
	ycrlfld->setValue( si_.b2c_.getTransform(false).c );
	overrulefld->setChecked( false );
    }
    else if ( !setRelation() )
	return false;

    survparchanged.trigger();
    return true;
}


void uiSurveyInfoEditor::doFinalise( CallBacker* )
{
    pathfld->setText( orgstorepath );
    pathfld->setReadOnly( true );
    updStatusBar( orgstorepath );

    SurveyInfo::Pol2D survtyp = si_.getSurvDataType();
    pol2dfld->box()->setCurrentItem( (int)survtyp );
    
    if ( si_.sampling(false).hrg.totalNr() )
	setValues();

    chgSetMode(0);
    ic1fld->setReadOnly( true, 0 );
}


bool uiSurveyInfoEditor::rejectOK( CallBacker* )
{
    if ( isnew )
    {
	BufferString dirnm = FilePath(orgstorepath).add(orgdirname).fullPath();
	if ( File_exists(dirnm) )
	    File_remove( dirnm, mFile_Recursive );
    }
    return true;
}

bool uiSurveyInfoEditor::setSurvName()
{
    BufferString newsurvnm( survnmfld->text() );
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

    const BufferString newstorepath( pathfld->text() );
    const BufferString newdirnm( dirName() );
    const BufferString olddir(
	    		FilePath(orgstorepath).add(orgdirname).fullPath() );
    const BufferString newdir( FilePath(newstorepath).add(newdirnm).fullPath());
    const bool storepathchanged = orgstorepath != newstorepath;
    dirnamechanged = orgdirname != newdirnm;

    if ( !isnew )
    {
	if ( (dirnamechanged || storepathchanged)
	  && File_exists(newdir) )
	{
	    uiMSG().error( "The new target directory exists.\n"
		    	   "Please enter another directory name or location." );
	    return false;
	}

	if ( storepathchanged )
	{
	    if ( !uiMSG().askGoOn("Copy your survey to another location?") )
		return false;
	    else if ( !copySurv(orgstorepath,orgdirname,newstorepath,newdirnm) )
		return false;
	    else if ( !uiMSG().askGoOn("Keep the survey at the old location?") )
		File_remove( olddir, mFile_Recursive );
	}
	else if ( dirnamechanged )
	{
	    if ( !renameSurv(orgstorepath,orgdirname,newdirnm) )
		return false;
	}
    }
    else
    {
	if ( File_exists(newdir) )
	{
	    uiMSG().error( "The chosen target directory exists.\n"
		    	   "Please enter another name or location." );
	    return false;
	}

	if ( newstorepath != orgstorepath )
	{
	    if ( !copySurv(orgstorepath,orgdirname,newstorepath,newdirnm) )
		return false;
	    File_remove( olddir, mFile_Recursive );
	}
	else if ( !renameSurv(newstorepath,orgdirname,newdirnm) )
	    return false;
    }

    BufferString linkpos = FilePath(rootdir).add(newdirnm).fullPath(); 
    if ( File_exists(linkpos) )
    {
       if ( File_isLink(linkpos) )
	   File_remove( linkpos, mFile_NotRecursive );
    }

    if ( !File_exists(linkpos) )
    {
	if ( !File_createLink(newdir,linkpos) )
	{
	    BufferString msg( "Cannot create link from \n" );
	    msg += newdir; msg += " to \n"; msg += linkpos;
	    uiMSG().error( msg ); 
	    return false;
	}
    }

    si_.dirname = newdirnm;
    si_.setSurvDataType( (SurveyInfo::Pol2D)pol2dfld->box()->currentItem() );
    if ( mUseAdvanced() )
	si_.get3Pts( si_.set3coords, si_.set3binids, si_.set3binids[2].crl );

    if ( !si_.write(rootdir) )
    {
        uiMSG().error( "Failed to write survey info.\nNo changes committed." );
	return false;
    }
    
    return true;
}


const char* uiSurveyInfoEditor::dirName() const
{
    static BufferString ret; ret = survnmfld->text();
    cleanupString( ret.buf(), mC_False, mC_False, mC_True );
    return ret.buf();
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSurveyInfoEditor::setRanges()
{
    StepInterval<int> irg( inlfld->getIStepInterval() );
    StepInterval<int> crg( crlfld->getIStepInterval() );
    CubeSampling cs( si_.sampling(false) );
    HorSampling& hs = cs.hrg;
    hs.start.inl = irg.start; hs.start.crl = crg.start;
    hs.stop.inl = irg.stop;   hs.stop.crl = crg.stop;
    hs.step.inl = irg.step;   hs.step.crl = crg.step;
    if ( hs.step.inl < 1 ) hs.step.inl = 1;
    if ( hs.step.crl < 1 ) hs.step.crl = 1;

    const int curzunititem = zunitfld->box()->currentItem();
    si_.setZUnit( curzunititem == 0, curzunititem == 1 );
    cs.zrg = zfld->getFStepInterval();
    if ( mIsUdf(cs.zrg.start) || mIsUdf(cs.zrg.stop) || mIsUdf(cs.zrg.step) )
	mErrRet("Please enter the Z Range")
    const float zfac = 1. / si_.zFactor();
    if ( !mIsEqual(zfac,1,0.0001) )
	{ cs.zrg.start *= zfac; cs.zrg.stop *= zfac; cs.zrg.step *= zfac; }
    if ( mIsZero(cs.zrg.step,1e-8) )
	cs.zrg.step = si_.zIsTime() ? 0.004 : 1;
    cs.normalise();
    if ( !hs.totalNr() )
	mErrRet("Please specify inline/crossline ranges")
    if ( cs.zrg.nrSteps() == 0 )
	mErrRet("Please specify a valid Z range")

    si_.setRange( cs, false );
    const bool inft = xyinftfld->isChecked();
    if ( inft )
	uiMSG().warning(
		"Support for coordinates in feet has been added recently.\n"
		"The impact of it is still being evaluated.\n"
		"Please be on guard for unprepared parts of the software" );
    si_.setXYInFeet( inft );
    return true;
}


bool uiSurveyInfoEditor::setCoords()
{
    BinID b[2]; Coord c[3]; int xline;
    b[0] = ic0fld->getBinID();
    b[1] = ic2fld->getBinID();
    xline = ic1fld->getBinID().crl;
    c[0] = xy0fld->getCoord();
    c[1] = xy2fld->getCoord();
    c[2] = xy1fld->getCoord();
  
    const char* msg = si_.set3Pts( c, b, xline );
    if ( msg ) { uiMSG().error( msg ); return false; }

    return true;
}


bool uiSurveyInfoEditor::setRelation()
{
    RCol2Coord::RCTransform xtr, ytr;
    xtr.a = x0fld->getdValue();   ytr.a = y0fld->getdValue();
    xtr.b = xinlfld->getdValue(); ytr.b = yinlfld->getdValue();
    xtr.c = xcrlfld->getdValue(); ytr.c = ycrlfld->getdValue();
    if ( !xtr.valid(ytr) )
    {
        uiMSG().error( "The transformation is not valid." );
        return false;
    }

    si_.b2c_.setTransforms( xtr, ytr );
    return true;
}


void uiSurveyInfoEditor::sipbutPush( CallBacker* cb )
{
    const int sipidx = sipfld ? sipfld->currentItem() : sipbuts.indexOf( cb );
    if ( sipidx < 0 ) { pErrMsg("Huh?"); return; }
    delete impiop_; impiop_ = 0; lastsip_ = 0;

    const int curzunititem = zunitfld->box()->currentItem();
    si_.setZUnit( curzunititem == 0, curzunititem == 1 );

    uiSurvInfoProvider* sip = survInfoProvs()[sipidx];
    PtrMan<uiDialog> dlg = sip->dialog( this );
    if ( !dlg || !dlg->go() ) return;

    CubeSampling cs; Coord crd[3];
    if ( !sip->getInfo(dlg,cs,crd) )
	return;

    if ( sip->tdInfo() != uiSurvInfoProvider::Uknown )
	si_.setZUnit( sip->tdInfo() == uiSurvInfoProvider::Time,
		      sip->tdInfo() == uiSurvInfoProvider::DepthFeet );
    si_.setXYInFeet( sip->xyInFeet() );

    if ( mIsUdf(cs.zrg.start) )
	cs.zrg = si_.zRange(false);

    si_.setRange(cs,false);
    BinID bid[2];
    bid[0].inl = cs.hrg.start.inl; bid[0].crl = cs.hrg.start.crl;
    bid[1].inl = cs.hrg.stop.inl; bid[1].crl = cs.hrg.stop.crl;
    si_.set3Pts( crd, bid, cs.hrg.stop.crl );
    setValues();

    si_.setWSProjName( SI().getWSProjName() );
    si_.setWSPwd( SI().getWSPwd() );

    lastsip_ = sip;
    impiop_ = lastsip_->getImportPars();
}


void uiSurveyInfoEditor::pathbutPush( CallBacker* )
{
    uiFileDialog dlg( this, uiFileDialog::DirectoryOnly, pathfld->text() );
    if ( dlg.go() )
    {
	BufferString dirnm( dlg.fileName() );
	if ( !File_isWritable(dirnm) )
	{
	    uiMSG().error( "Directory is not writable" );
	    return;
	}
	updStatusBar( dirnm );
	pathfld->setText( dirnm );
    }
}


void uiSurveyInfoEditor::updStatusBar( const char* dirnm )
{
    BufferString msg;
    GetFreeMBOnDiskMsg( File_getFreeMBytes(dirnm), msg );
    toStatusBar( msg );
}


void uiSurveyInfoEditor::chgSetMode( CallBacker* )
{
    crdgrp->display( coordset->getBoolValue() );
    trgrp->display( !coordset->getBoolValue() );
}


void uiSurveyInfoEditor::setInl1Fld( CallBacker* )
{
    ic1fld->setText( ic0fld->text(0), 0 );
}
