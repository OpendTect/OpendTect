/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.cc,v 1.46 2003-10-27 23:10:02 bert Exp $
________________________________________________________________________

-*/

#include "uisurvinfoed.h"
#include "errh.h"
#include "filegen.h"
#include "survinfoimpl.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uisurvey.h"
#include "uimsg.h"
#include "uifiledlg.h"
#include "ioobj.h" // for GetFreeMBOnDiskMsg
#include "ptrman.h"

extern "C" const char* GetBaseDataDir();

static ObjectSet<uiSurvInfoProvider>& survInfoProvs()
{
    static ObjectSet<uiSurvInfoProvider>* sips = 0;
    if ( !sips )
	sips = new ObjectSet<uiSurvInfoProvider>;
    return *sips;
}


int uiSurveyInfoEditor::addInfoProvider( uiSurvInfoProvider* p )
{
    if ( p ) survInfoProvs() += p;
    return survInfoProvs().size();
}


uiSurveyInfoEditor::uiSurveyInfoEditor( uiParent* p, SurveyInfo* si_, 
					const CallBack& appcb )
    : uiDialog(p,uiDialog::Setup("Survey setup",
				 "Specify survey parameters","0.3.2")
				 .nrstatusflds(1))
    , rootdir( GetBaseDataDir() )
    , dirnmch_(0)
    , survinfo(si_)
    , survparchanged(this)
    , x0fld(0)
    , orgdirname(si_ ? (const char*)si_->dirname : "")
{
    if ( !si_ ) return;

    dirnmfld = new uiGenInput( this, "Survey short name (directory name)", 
			       StringInpSpec( orgdirname ) );
    survnmfld = new uiGenInput( this, "Full Survey name",
	    			StringInpSpec(survinfo->name()) );
    survnmfld->attach( alignedBelow, dirnmfld );

    pathfld = new uiGenInput( this, "Location on disk",
	    			StringInpSpec( rootdir ) );
    pathfld->attach( alignedBelow, survnmfld );
    uiButton* pathbut;
    if ( !orgdirname.size() )
    {
	pathbut = new uiPushButton( this, "Select" );
	pathbut->attach( rightOf, pathfld );
	pathbut->activated.notify( mCB(this,uiSurveyInfoEditor,pathbutPush) );
    }

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->attach( stretchedBelow, pathfld, -2 );

    uiObject* prevobj = pathfld->attachObj();
    if ( survInfoProvs().size() )
    {
	for ( int idx=0; idx<survInfoProvs().size(); idx++ )
	{
	    BufferString txt( "Fetch setup from " );
	    txt += survInfoProvs()[idx]->name();
	    txt += " ...";
	    uiPushButton* newpb = new uiPushButton( this, txt );
	    sipbuts += newpb;
	    newpb->attach( idx == 0 ? alignedBelow : rightOf, prevobj );
	    newpb->activated.notify( mCB(this,uiSurveyInfoEditor,sipbutPush) );
	    prevobj = newpb;
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
    rangegrp->attach( alignedBelow, pathfld ); 
    rangegrp->attach( ensureBelow, rglbl ); 
    if ( prevobj ) rangegrp->attach( ensureBelow, prevobj ); 
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

    if ( survinfo->is3D() )
    {
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
	ic1fld = new uiGenInput( crdgrp, "Cross-line on above in-line",
				 BinIDCoordInpSpec(false)  );
	ic2fld = new uiGenInput( crdgrp,
			"In-line/Cross-line not on above in-line",
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
    }

    finaliseDone.notify( mCB(this,uiSurveyInfoEditor,doFinalise) );
    survparchanged.notify( appcb );
}


void uiSurveyInfoEditor::setValues()
{
    const BinIDRange br = survinfo->range( false );
    const BinID bs = BinID( survinfo->inlStep(), survinfo->crlStep() );
    StepInterval<int> inlrg( br.start.inl, br.stop.inl, bs.inl );
    StepInterval<int> crlrg( br.start.crl, br.stop.crl, bs.crl );
    inlfld->setValue( inlrg );
    crlfld->setValue( crlrg );
    if ( survinfo->zRangeUsable() )
    {
	bool zistime = survinfo->zIsTime();
	StepInterval<double> zrg = survinfo->zRange( false );
	if ( zistime )
	{
	    zfld->setValue( mNINT(zrg.start*1000), 0 );
	    zfld->setValue( mNINT(zrg.stop*1000), 1 );
	    zfld->setValue( mNINT(zrg.step*1000), 2 );
	}
	else
	    zfld->setValue( zrg );

	timefld->setChecked( zistime );
	meterfld->setChecked( survinfo->zInMeter() );
	feetfld->setChecked( survinfo->zInFeet() );
    }

    if ( x0fld )
    {
	SurveyInfo3D& si = *(SurveyInfo3D*)survinfo;
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
	    xy0fld->setValue( c[0] );
	    xy1fld->setValue( c[2] );
	    xy2fld->setValue( c[1] );
	}
    }
}


bool uiSurveyInfoEditor::appButPushed()
{
    if ( !x0fld || !setRanges() ) return false;

    if ( !overrule->isChecked() || coordset->getBoolValue() )
    {
	if ( !setCoords() ) return false;

	SurveyInfo3D& si = *(SurveyInfo3D*)survinfo;
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
    if ( orgdirname.size() )
    {
	BufferString from = File_getFullPath( rootdir, orgdirname );
	BufferString path = File_getPathOnly( File_linkTarget( from ) );
	pathfld->setText( path );
	pathfld->setReadOnly( true );
	updStatusBar( path );
    }

    if ( survinfo->rangeUsable() ) setValues();
    if ( !x0fld ) return;

    chgSetMode(0);
//  if( ic1fld->uiObj() ) ic1fld->uiObj()->setSensitive( false );
    ic1fld->setReadOnly( true, 0 );

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

    if ( !appButPushed() )
	return false;

    if ( orgdirname == "" )
    {
	BufferString from( GetSoftwareDir() );
	from = File_getFullPath( GetSoftwareDir(), "data" );
	from = File_getFullPath( from, "BasicSurvey" );

	BufferString to( pathfld->text() );
	to = File_getFullPath( to, newdirnm );
	if ( File_exists(to) )
	{
	    BufferString errmsg( "Please rename survey.\n\n '");
	    errmsg += newdirnm; errmsg += "' already exists!";
	    uiMSG().error( errmsg ); 
	    return false;
	}

	if ( !File_copy( from, to, YES ) )
	{
	    uiMSG().error( "Cannot create proper new survey directory" );
	    return false;
	}

	File_makeWritable( to, YES, YES );
	survinfo->dirname = newdirnm;
	BufferString link = File_getFullPath( rootdir, newdirnm ); 
	if ( link != to )
	    if ( !File_createLink( to, link ) )
	    {
		BufferString msg( "Cannot create link from \n" );
		msg += to; msg += " to \n"; msg += link;
		uiMSG().error( msg ); 
		return false;
	    }
    }
    else
        if ( orgdirname != newdirnm ) dirnmch_ = true;

    if ( !survinfo->write( rootdir ) )
        uiMSG().error( "Failed to write survey info.\nNo changes committed." );
    else
    {
        delete SurveyInfo::theinst_;
        SurveyInfo::theinst_ = survinfo;
    }
    
    survinfo->dirname = dirnmfld->text();

    return true;
}


const char* uiSurveyInfoEditor::dirName()
{
    orgdirname = dirnmfld->text();
    cleanupString( orgdirname.buf(), NO, NO, YES );
    return orgdirname;
}


bool uiSurveyInfoEditor::setRanges()
{
    BufferString survnm( survnmfld->text() );
    if ( survnm == "" ) survnm = dirnmfld->text();
    survinfo->setName( survnm );

    StepInterval<int> irg( inlfld->getIStepInterval() );
    StepInterval<int> crg( crlfld->getIStepInterval() );
    BinIDRange br;
    br.start.inl = irg.start; br.start.crl = crg.start;
    br.stop.inl = irg.stop;   br.stop.crl = crg.stop;
    survinfo->setRange( br, true ); survinfo->setRange( br, false );
    if ( !survinfo->rangeUsable() )
    { 
	uiMSG().error( "Please specify inline/crossline ranges" ); 
        return false; 
    }

    survinfo->setZUnit( timefld->isChecked(), meterfld->isChecked() );
    StepInterval<double> zrs( zfld->getDStepInterval() );
    if ( survinfo->zIsTime() )
    {
	zrs.start /= 1000; zrs.stop /= 1000; zrs.step /= 1000;
    }
    survinfo->setZRange( zrs, true ); survinfo->setZRange( zrs, false );
    if ( !survinfo->zRangeUsable() )
    {
	uiMSG().error( "Please specify time range" );
	return false;
    }

    if ( !x0fld ) return true;

    BinID bs( irg.step, crg.step );
    if ( !bs.inl ) bs.inl = 1; if ( !bs.crl ) bs.crl = 1;
    survinfo->setStep( bs, true ); survinfo->setStep( bs, false );

    return true;
}


bool uiSurveyInfoEditor::setCoords()
{
    if ( !x0fld ) return true;

    BinID b[2]; Coord c[3]; int xline;
    b[0] = ic0fld->getBinID();
    b[1] = ic2fld->getBinID();
    xline = ic1fld->getBinID().crl;
    c[0] = xy0fld->getCoord();
    c[1] = xy2fld->getCoord();
    c[2] = xy1fld->getCoord();
  
    const char* msg = ((SurveyInfo3D*)survinfo)->set3Pts( c, b, xline );
    if ( msg ) { uiMSG().error( msg ); return false; }

    return true;
}


bool uiSurveyInfoEditor::setRelation()
{
    if ( !x0fld ) return true;

    BinID2Coord::BCTransform xtr, ytr;
    xtr.a = x0fld->getValue();   ytr.a = y0fld->getValue();
    xtr.b = xinlfld->getValue(); ytr.b = yinlfld->getValue();
    xtr.c = xcrlfld->getValue(); ytr.c = ycrlfld->getValue();
    if ( !xtr.valid(ytr) )
    {
        uiMSG().error( "The transformation is not valid." );
        return false;
    }

    ((SurveyInfo3D*)survinfo)->b2c_.setTransforms( xtr, ytr );
    return true;
}


void uiSurveyInfoEditor::sipbutPush( CallBacker* cb )
{
    int sipidx = sipbuts.indexOf( cb );
    if ( sipidx < 0 ) { pErrMsg("Huh?"); return; }

    uiSurvInfoProvider* sip = survInfoProvs()[sipidx];
    PtrMan<uiDialog> dlg = sip->dialog( this );
    if ( !dlg || !dlg->go() ) return;

    BinIDSampler bs; StepInterval<double> zrg; Coord crd[3];
    if ( !sip->getInfo(dlg,bs,zrg,crd) )
	return;

    survinfo->setRange(bs,true); survinfo->setRange(bs,false);
    survinfo->setStep(bs.step,true); survinfo->setStep(bs.step,false);
    survinfo->setZRange(zrg,true); survinfo->setZRange(zrg,false);
    if ( x0fld )
    {
	BinID bid[2];
	bid[0].inl = bs.start.inl; bid[0].crl = bs.start.crl;
	bid[1].inl = bs.stop.inl; bid[1].crl = bs.stop.crl;
	((SurveyInfo3D*)survinfo)->set3Pts( crd, bid, bs.stop.crl );
    }
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
    if ( !x0fld ) return;
    crdgrp->display( coordset->getBoolValue() );
    trgrp->display( !coordset->getBoolValue() );
}


void uiSurveyInfoEditor::setInl1Fld( CallBacker* )
{
    if ( !x0fld ) return;
    ic1fld->setText( ic0fld->text(0), 0 );
}
