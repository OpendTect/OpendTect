/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.cc,v 1.64 2004-10-22 14:38:05 bert Exp $
________________________________________________________________________

-*/

#include "uisurvinfoed.h"
#include "errh.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uisurvey.h"
#include "uimsg.h"
#include "uicursor.h"
#include "uimain.h"
#include "uifiledlg.h"
#include "ioobj.h" // for GetFreeMBOnDiskMsg
#include "ioman.h"
#include "filegen.h"
#include "filepath.h"
#include "cubesampling.h"
#include "ptrman.h"

extern "C" const char* GetBaseDataDir();



static ObjectSet<uiSurvInfoProvider>& survInfoProvs()
{
    static ObjectSet<uiSurvInfoProvider>* sips = 0;
    if ( !sips )
	sips = new ObjectSet<uiSurvInfoProvider>;
    return *sips;
}


uiSurveyInfoEditor::uiSurveyInfoEditor( uiParent* p, SurveyInfo* si_ )
	: uiDialog(p,uiDialog::Setup("Survey setup",
				     "Specify survey parameters","0.3.2")
				     .nrstatusflds(1))
	, rootdir(GetBaseDataDir())
	, orgdirname(si_?si_->dirname.buf():"")
	, survinfo(si_)
	, survparchanged(this)
	, x0fld(0)
	, dirnamechanged(false)
	, globcurdirname(SI().dirname)
	, sipfld(0)
{
    if ( !survinfo ) return;

    orgstorepath = survinfo ? survinfo->datadir.buf() : rootdir.buf();
    isnew = !survinfo || orgdirname == "";

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
    }
    else
    {
	orgstorepath = rootdir;
	orgdirname = newSurvTempDirName();
	BufferString dirnm = FilePath( orgstorepath )
	    		    .add( orgdirname ).fullPath();
	if ( File_exists(dirnm) )
	    File_remove( dirnm, YES );
	if ( !copySurv(GetDataFileName(0),"BasicSurvey",
		       orgstorepath,orgdirname) )
	    return;
	File_makeWritable( dirnm, YES, YES );
    }
    newSurvey( orgdirname );

    dirnmfld = new uiGenInput( this, "Survey directory name", 
			       StringInpSpec( isnew ? "" : orgdirname.buf()) );
    survnmfld = new uiGenInput( this, "Full Survey name",
	    			StringInpSpec(survinfo->name()) );
    survnmfld->attach( alignedBelow, dirnmfld );

    pathfld = new uiGenInput( this, "Location on disk",
	    			StringInpSpec(orgstorepath) );
    pathfld->attach( alignedBelow, survnmfld );

#ifdef __win__
    pathfld->setSensitive( false );
#else
    uiButton* pathbut = new uiPushButton( this, "Select" );
    pathbut->attach( rightOf, pathfld );
    pathbut->activated.notify( mCB(this,uiSurveyInfoEditor,pathbutPush) );
#endif

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->attach( stretchedBelow, pathfld, -2 );

    const int nrprovs = survInfoProvs().size();
    uiObject* sipobj = 0;
    if ( nrprovs > 0 )
    {
	CallBack sipcb( mCB(this,uiSurveyInfoEditor,sipbutPush) );
	int maxlen = 0;
	for ( int idx=0; idx<nrprovs; idx++ )
	{
	    int len = strlen( survInfoProvs()[idx]->usrText() );
	    if ( len > maxlen ) maxlen = len;
	}

	if ( nrprovs > 2 )
	{
	    sipobj = sipfld = new uiComboBox( this, "SIPs" );
	    sipfld->attach( alignedBelow, pathfld );
	    sipfld->attach( ensureBelow, horsep1 );
	    for ( int idx=0; idx<nrprovs; idx++ )
	    {
		BufferString txt( survInfoProvs()[idx]->usrText() );
		txt += " ->>";
		sipfld->addItem( txt );
	    }
	    sipfld->setCurrentItem( nrprovs-1 );
	    sipfld->setPrefWidthInChar( maxlen + 10 );
	    uiPushButton* sipbut = new uiPushButton( this, "Go ...", sipcb );
	    sipbut->attach( rightOf, sipfld );
	}
	else
	{
	    uiPushButton* prevbut = 0;
	    for ( int idx=0; idx<nrprovs; idx++ )
	    {
		BufferString txt( survInfoProvs()[idx]->usrText() );
		txt += " ...";
		uiPushButton* newpb = new uiPushButton( this, txt, sipcb );
		sipbuts += newpb;
		if ( prevbut )
		    newpb->attach( rightOf, prevbut );
		else
		{
		    newpb->attach( alignedBelow, pathfld );
		    newpb->attach( ensureBelow, horsep1 );
		    sipobj = newpb;
		}
		newpb->setPrefWidthInChar( maxlen + 6 );
		prevbut = newpb;
	    }
	}
    }

    uiLabel* rglbl = new uiLabel( this, "Survey ranges:" );
    rglbl->attach( leftBorder );
    rglbl->attach( ensureBelow, horsep1 );
    uiGroup* rangegrp = new uiGroup( this, "Survey ranges" );
    inlfld = new uiGenInput( rangegrp, "In-line range",
			     IntInpIntervalSpec(true) );
    crlfld = new uiGenInput( rangegrp, "Cross-line range",
			     IntInpIntervalSpec(true) );
    zfld = new uiGenInput( rangegrp, "Z range", DoubleInpIntervalSpec(true) );
    rangegrp->setHAlignObj( inlfld );
    if ( sipobj )
	rangegrp->attach( alignedBelow, sipobj ); 
    else
    {
	rangegrp->attach( alignedBelow, pathfld ); 
	rangegrp->attach( ensureBelow, rglbl ); 
    }
    crlfld->attach( alignedBelow, inlfld );
    zfld->attach( alignedBelow, crlfld );

    timefld = new uiRadioButton( rangegrp, "msec" );
    timefld->setChecked( true );
    timefld->attach( alignedBelow, zfld );
    timefld->activated.notify( mCB(this,uiSurveyInfoEditor,unitPush) );
    meterfld = new uiRadioButton( rangegrp, "meter" );
    meterfld->attach( rightTo, timefld );
    meterfld->activated.notify( mCB(this,uiSurveyInfoEditor,unitPush) );
    feetfld = new uiRadioButton( rangegrp, "feet" );
    feetfld->attach( rightTo,meterfld );
    feetfld->activated.notify( mCB(this,uiSurveyInfoEditor,unitPush) );
    uiLabel* unitlbl = new uiLabel( rangegrp, "Unit" );
    unitlbl->attach( leftOf, timefld );

    uiSeparator* horsep2 = new uiSeparator( this );
    horsep2->attach( stretchedBelow, rangegrp );

    uiLabel* crdlbl = new uiLabel( this, "Coordinate settings:" );
    crdlbl->attach( leftBorder );
    crdlbl->attach( ensureBelow, horsep2 );
    coordset = new uiGenInput( this, "", BoolInpSpec( "Easy", "Advanced" ));
    coordset->attach( alignedBelow, rangegrp );
    coordset->attach( rightTo, crdlbl );
    coordset->valuechanged.notify( mCB(this,uiSurveyInfoEditor,chgSetMode));

    crdgrp = new uiGroup( this, "Coordinate settings" );
    ic0fld = new uiGenInput( crdgrp, "First In-line/Cross-line", 
			     BinIDCoordInpSpec(false) ); 
    ic0fld->valuechanging.notify( mCB(this,uiSurveyInfoEditor,setInl1Fld) );
    ic1fld = new uiGenInput( crdgrp, "Another position on above in-line",
			     BinIDCoordInpSpec(false)  );
    ic2fld = new uiGenInput( crdgrp,
		    "Position not on above in-line",
		     BinIDCoordInpSpec(false) );
    xy0fld = new uiGenInput( crdgrp, "= (X,Y)", BinIDCoordInpSpec(true) );
    xy1fld = new uiGenInput( crdgrp, "= (X,Y)", BinIDCoordInpSpec(true) );
    xy2fld = new uiGenInput( crdgrp, "= (X,Y)", BinIDCoordInpSpec(true) );
    crdgrp->setHAlignObj( ic0fld );
    crdgrp->attach( alignedBelow, rangegrp );
    crdgrp->attach( ensureBelow, coordset );
    ic1fld->attach( alignedBelow, ic0fld );
    ic2fld->attach( alignedBelow, ic1fld );
    xy0fld->attach( rightOf, ic0fld );
    xy1fld->attach( rightOf, ic1fld );
    xy2fld->attach( rightOf, ic2fld );

    trgrp = new uiGroup( this, "I/C to X/Y transformation" );
    x0fld = new uiGenInput ( trgrp, "X = ", DoubleInpSpec() );
    x0fld->setElemSzPol( uiObject::small );
    xinlfld = new uiGenInput ( trgrp, "+ in-line *", DoubleInpSpec() );
    xinlfld->setElemSzPol( uiObject::small );
    xcrlfld = new uiGenInput ( trgrp, "+ cross-line *", DoubleInpSpec() );
    xcrlfld->setElemSzPol( uiObject::small );
    y0fld = new uiGenInput ( trgrp, "Y = ", DoubleInpSpec() );
    y0fld->setElemSzPol( uiObject::small );
    yinlfld = new uiGenInput ( trgrp, "+ in-line *", DoubleInpSpec() );
    yinlfld->setElemSzPol( uiObject::small );
    ycrlfld = new uiGenInput ( trgrp, "+ cross-line *", DoubleInpSpec() );
    ycrlfld->setElemSzPol( uiObject::small );
    overrule= new uiCheckBox( trgrp, "Overrule easy settings" );
    overrule->setChecked( false );
    trgrp->setHAlignObj( xinlfld );
    trgrp->attach( alignedBelow, rangegrp );
    trgrp->attach( ensureBelow, coordset );
    xinlfld->attach( rightOf, x0fld );
    xcrlfld->attach( rightOf, xinlfld );
    y0fld->attach( alignedBelow, x0fld );
    yinlfld->attach( rightOf, y0fld );
    ycrlfld->attach( rightOf, yinlfld );
    overrule->attach( alignedBelow, ycrlfld );

    applybut = new uiPushButton( this, "Apply" ); 
    applybut->activated.notify( mCB(this,uiSurveyInfoEditor,appButPushed) );
    applybut->attach( alignedBelow, crdgrp );

    finaliseDone.notify( mCB(this,uiSurveyInfoEditor,doFinalise) );
}


static void setZValFld( uiGenInput* zfld, int nr, float val, float fac )
{
    if ( mIsUndefined(val) )
	{ zfld->setText( "", nr ); return; }

    val *= fac; int ival = mNINT(val); float fival = ival;
    if ( mIsEqual(val,fival,0.01) )
	zfld->setValue( ival, nr );
    else
	zfld->setValue( val, nr );
}


void uiSurveyInfoEditor::setValues()
{
    const SurveyInfo& si = *survinfo;
    const CubeSampling& cs = si.sampling( false );
    const HorSampling& hs = cs.hrg;
    StepInterval<int> inlrg( hs.start.inl, hs.stop.inl, hs.step.inl );
    StepInterval<int> crlrg( hs.start.crl, hs.stop.crl, hs.step.crl );
    inlfld->setValue( inlrg );
    crlfld->setValue( crlrg );

    const StepInterval<float>& zrg = si.zRange( false );
    const float zfac = si.zFactor();
    setZValFld( zfld, 0, zrg.start, zfac );
    setZValFld( zfld, 1, zrg.stop, zfac );
    setZValFld( zfld, 2, zrg.step, zfac );

    timefld->setChecked( si.zIsTime() );
    meterfld->setChecked( si.zInMeter() );
    feetfld->setChecked( si.zInFeet() );

    x0fld->setValue( si.b2c_.getTransform(true).a );
    xinlfld->setValue( si.b2c_.getTransform(true).b );
    xcrlfld->setValue( si.b2c_.getTransform(true).c );
    y0fld->setValue( si.b2c_.getTransform(false).a );
    yinlfld->setValue( si.b2c_.getTransform(false).b );
    ycrlfld->setValue( si.b2c_.getTransform(false).c );

    Coord c[3]; BinID b[2]; int xline;
    si.get3Pts( c, b, xline );
    if ( b[0].inl )
    {
	ic0fld->setValue( b[0] );
	ic1fld->setValues( b[0].inl, xline );
	ic2fld->setValue( b[1] );
	if ( !c[0].x && !c[0].y && !c[1].x && !c[1].y && !c[2].x && !c[2].y)
	{
	    c[0] = si.transform( b[0] );
	    c[1] = si.transform( b[1] );
	    c[2] = si.transform( BinID(b[0].inl,xline) );
	}
	xy0fld->setValue( c[0] );
	xy1fld->setValue( c[2] );
	xy2fld->setValue( c[1] );
    }
}


void uiSurveyInfoEditor::newSurvey( const char* nm )
{
    delete SurveyInfo::theinst_;
    SurveyInfo::theinst_ = 0;
    IOMan::setSurvey( nm );
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
    if ( usr ) nm += usr;
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
    uiCursor::setOverride( uiCursor::Wait );
    File_copy( fnmin, fnmout, YES );
    uiCursor::restoreOverride();
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


bool uiSurveyInfoEditor::appButPushed()
{
    if ( !setRanges() ) return false;

    if ( !overrule->isChecked() || coordset->getBoolValue() )
    {
	if ( !setCoords() ) return false;

	SurveyInfo& si = *survinfo;
	x0fld->setValue( si.b2c_.getTransform(true).a );
	xinlfld->setValue( si.b2c_.getTransform(true).b );
	xcrlfld->setValue( si.b2c_.getTransform(true).c );
	y0fld->setValue( si.b2c_.getTransform(false).a );
	yinlfld->setValue( si.b2c_.getTransform(false).b );
	ycrlfld->setValue( si.b2c_.getTransform(false).c );
	overrule->setChecked( false );
    }
    else
	if ( !setRelation() ) return false;

    survparchanged.trigger();
    return true;
}


void uiSurveyInfoEditor::doFinalise( CallBacker* )
{
    pathfld->setText( orgstorepath );
    pathfld->setReadOnly( true );
    updStatusBar( orgstorepath );

    if ( survinfo->sampling(false).hrg.totalNr() )
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
	    File_remove( dirnm, YES );
    }
    newSurvey( globcurdirname );
    return true;
}

bool uiSurveyInfoEditor::acceptOK( CallBacker* )
{
    const char* newdirnminp = dirnmfld->text();
    if ( !newdirnminp || !*newdirnminp )
    {
	uiMSG().error( "Please specify the short survey (directory) name." );
	return false;
    }
    BufferString newdirnm = newdirnminp;
    cleanupString( newdirnm.buf(), NO, YES, YES );
    if ( newdirnm != newdirnminp )
	dirnmfld->setText( newdirnm );

    dirnamechanged = orgdirname != newdirnm;
    survinfo->dirname = newdirnm;

    if ( !appButPushed() )
	return false;

    BufferString newstorepath = pathfld->text();

    BufferString olddir = FilePath(orgstorepath).add(orgdirname).fullPath();
    BufferString newdir = FilePath(newstorepath).add(newdirnm).fullPath();
    const bool storepathchanged = orgstorepath != newstorepath;

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
		File_remove( olddir, YES );
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
	    File_remove( olddir, YES );
	}
	else if ( !renameSurv(newstorepath,orgdirname,newdirnm) )
	    return false;
    }

    survinfo->dirname = newdirnm;
    BufferString linkpos = FilePath(rootdir).add(newdirnm).fullPath(); 
    if ( File_exists(linkpos) )
    {
       if ( File_isLink(linkpos) )
	   File_remove( linkpos, NO );
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

    if ( !survinfo->write(rootdir) )
    {
        uiMSG().error( "Failed to write survey info.\nNo changes committed." );
	return false;
    }
    
    newSurvey( globcurdirname );
    return true;
}


const char* uiSurveyInfoEditor::dirName()
{
    orgdirname = dirnmfld->text();
    cleanupString( orgdirname.buf(), NO, NO, YES );
    return orgdirname;
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSurveyInfoEditor::setRanges()
{
    BufferString survnm( survnmfld->text() );
    if ( survnm == "" ) survnm = dirnmfld->text();
    survinfo->setName( survnm );

    StepInterval<int> irg( inlfld->getIStepInterval() );
    StepInterval<int> crg( crlfld->getIStepInterval() );
    CubeSampling cs( survinfo->sampling() );
    HorSampling& hs = cs.hrg;
    hs.start.inl = irg.start; hs.start.crl = crg.start;
    hs.stop.inl = irg.stop;   hs.stop.crl = crg.stop;
    hs.step.inl = irg.step;   hs.step.crl = crg.step;
    if ( hs.step.inl < 1 ) hs.step.inl = 1;
    if ( hs.step.crl < 1 ) hs.step.crl = 1;

    survinfo->setZUnit( timefld->isChecked(), meterfld->isChecked() );
    cs.zrg = zfld->getFStepInterval();
    if ( mIsUndefined(cs.zrg.start) || mIsUndefined(cs.zrg.stop)
      || mIsUndefined(cs.zrg.step) )
	mErrRet("Please enter the Z Range")
    const float zfac = 1. / survinfo->zFactor();
    if ( !mIsEqual(zfac,1,0.0001) )
	{ cs.zrg.start *= zfac; cs.zrg.stop *= zfac; cs.zrg.step *= zfac; }
    if ( mIsZero(cs.zrg.step,1e-8) )
	cs.zrg.step = survinfo->zIsTime() ? 0.004 : 1;
    cs.normalise();
    if ( !hs.totalNr() )
	mErrRet("Please specify inline/crossline ranges")
    if ( cs.zrg.nrSteps() == 0 )
	mErrRet("Please specify a valid Z range")

    survinfo->setRange( cs, false );
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
  
    const char* msg = survinfo->set3Pts( c, b, xline );
    if ( msg ) { uiMSG().error( msg ); return false; }

    return true;
}


bool uiSurveyInfoEditor::setRelation()
{
    BinID2Coord::BCTransform xtr, ytr;
    xtr.a = x0fld->getValue();   ytr.a = y0fld->getValue();
    xtr.b = xinlfld->getValue(); ytr.b = yinlfld->getValue();
    xtr.c = xcrlfld->getValue(); ytr.c = ycrlfld->getValue();
    if ( !xtr.valid(ytr) )
    {
        uiMSG().error( "The transformation is not valid." );
        return false;
    }

    survinfo->b2c_.setTransforms( xtr, ytr );
    return true;
}


void uiSurveyInfoEditor::sipbutPush( CallBacker* cb )
{
    int sipidx = sipfld ? sipfld->currentItem() : sipbuts.indexOf( cb );
    if ( sipidx < 0 ) { pErrMsg("Huh?"); return; }

    survinfo->setZUnit( timefld->isChecked(), meterfld->isChecked() );

    uiSurvInfoProvider* sip = survInfoProvs()[sipidx];
    PtrMan<uiDialog> dlg = sip->dialog( this );
    if ( !dlg || !dlg->go() ) return;

    CubeSampling cs; Coord crd[3];
    if ( !sip->getInfo(dlg,cs,crd) )
	return;

    survinfo->setRange(cs,false);
    BinID bid[2];
    bid[0].inl = cs.hrg.start.inl; bid[0].crl = cs.hrg.start.crl;
    bid[1].inl = cs.hrg.stop.inl; bid[1].crl = cs.hrg.stop.crl;
    survinfo->set3Pts( crd, bid, cs.hrg.stop.crl );
    setValues();

    survinfo->setWSProjName( SI().getWSProjName() );
    survinfo->setWSPwd( SI().getWSPwd() );
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


void uiSurveyInfoEditor::unitPush( CallBacker* cb )
{
    mDynamicCastGet(uiRadioButton*,but,cb)
    if ( !but ) return;

    bool doms, domtr, doft;
    if ( but == timefld )
    { doms = true; domtr = doft = false; }
    else if ( but == meterfld )
    { domtr = true; doms = doft = false; }
    else if ( but == feetfld )
    { doft = true; domtr = doms = false; }

    timefld->setChecked( doms );
    meterfld->setChecked( domtr );
    feetfld->setChecked( doft );
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
