/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.cc,v 1.23 2002-01-16 14:14:50 nanne Exp $
________________________________________________________________________

-*/

#include "uisurvinfoed.h"
#include "errh.h"
#include "filegen.h"
#include "survinfo.h"
#include "idealconn.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uisurvey.h"
#include "uiidealdata.h"
#include "uimsg.h"

extern "C" const char* GetBaseDataDir();


uiSurveyInfoEditor::uiSurveyInfoEditor( uiParent* p, SurveyInfo* si, 
					const CallBack& appcb )
	: uiDialog(p,uiDialog::Setup("Survey setup",
		    		     "Specify survey parameters","0.3.2"))
	, rootdir( GetBaseDataDir() )
	, dirnmch_(0)
	, survinfo(si)
	, survparchanged(this)

{
    orgdirname = survinfo->dirname;
    survnm = survinfo->name();

    survnmfld = new uiGenInput( this, "Survey name", StringInpSpec( survnm ) );
    dirnmfld = new uiGenInput( this, "Directory name", 
			       StringInpSpec( orgdirname ) );
    BufferString nm = "( "; nm += rootdir; nm += "/ )";
    uiLabel* dirnm = new uiLabel( this, nm );
    dirnmfld->attach( alignedBelow, survnmfld );
    dirnm->attach( rightOf, dirnmfld );

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->attach( stretchedBelow, dirnmfld, -2 );

    BufferString txt( "Fetch setup from " );
    txt += IdealConn::guessedType() == IdealConn::SW
	 ? "SeisWorks ..." : "GeoFrame ...";
    uiButton* wsbut = 0;
    if ( IdealConn::haveIdealServices() )
    {
	wsbut = new uiPushButton( this, txt );
	wsbut->attach( alignedBelow, dirnmfld );
	wsbut->activated.notify( mCB(this,uiSurveyInfoEditor,wsbutPush) );
    }

    uiLabel* rglbl = new uiLabel( this, "Survey ranges:" );
    rglbl->attach( leftBorder );
    rglbl->attach( ensureBelow, horsep1 );
    uiGroup* rangegrp = new uiGroup( this, "Survey ranges" );
    inlfld = new uiGenInput( rangegrp, "In-line range",
			     IntInpIntervalSpec(true) );
    crlfld = new uiGenInput( rangegrp, "Cross-line range",
			     IntInpIntervalSpec(true) );
    zfld = new uiGenInput( rangegrp, "Time range (s)",
			   DoubleInpIntervalSpec(true) );
    rangegrp->setHAlignObj( inlfld->uiObj() );
    rangegrp->attach( alignedBelow, dirnmfld ); 
    rangegrp->attach( ensureBelow, rglbl ); 
    if ( wsbut ) rangegrp->attach( ensureBelow, wsbut ); 
    crlfld->attach( alignedBelow, inlfld );
    zfld->attach( alignedBelow, crlfld );

    uiSeparator* horsep2 = new uiSeparator( this );
    horsep2->attach( stretchedBelow, rangegrp );

    uiLabel* crdlbl = new uiLabel( this, "Coordinate settings:" );
    crdlbl->attach( leftBorder );
    crdlbl->attach( ensureBelow, horsep2 );
    coordset = new uiGenInput( this, "", BoolInpSpec( "Easy", "Advanced" ) );
    coordset->attach( alignedBelow, rangegrp );
    coordset->attach( rightTo, crdlbl );
    coordset->changed.notify( mCB(this,uiSurveyInfoEditor,chgSetMode) );

    DoubleInpSpec dis; dis.setHSzP(SzPolicySpec::small);
    crdgrp = new uiGroup( this, "Coordinate settings" );
    ic0fld = new uiGenInput( crdgrp, "First In-line/Cross-line", dis, dis ); 
    ic0fld->changed.notify( mCB(this,uiSurveyInfoEditor,setInl1Fld) );
    ic1fld = new uiGenInput( crdgrp, "Cross-line on above in-line", dis, dis );
    ic2fld = new uiGenInput( crdgrp, "In-line/Cross-line not on above in-line",
			     dis, dis );
    xy0fld = new uiGenInput( crdgrp, "= (X,Y)", BinIDCoordInpSpec(true) );
    xy1fld = new uiGenInput( crdgrp, "= (X,Y)", BinIDCoordInpSpec(true) );
    xy2fld = new uiGenInput( crdgrp, "= (X,Y)", BinIDCoordInpSpec(true) );
    crdgrp->setHAlignObj( ic0fld->uiObj() );
    crdgrp->attach( alignedBelow, rangegrp );
    crdgrp->attach( ensureBelow, coordset );
    ic1fld->attach( alignedBelow, ic0fld );
    ic2fld->attach( alignedBelow, ic1fld );
    xy0fld->attach( rightOf, ic0fld );
    xy1fld->attach( rightOf, ic1fld );
    xy2fld->attach( rightOf, ic2fld );

    trgrp = new uiGroup( this, "I/C to X/Y transformation" );
    x0fld = new uiGenInput ( trgrp, "X = ", dis );
    xinlfld = new uiGenInput ( trgrp, "+ in-line *", dis );
    xcrlfld = new uiGenInput ( trgrp, "+ cross-line *", dis );
    y0fld = new uiGenInput ( trgrp, "Y = ", dis );
    yinlfld = new uiGenInput ( trgrp, "+ in-line *", dis );
    ycrlfld = new uiGenInput ( trgrp, "+ cross-line *", dis );
    overrule= new uiCheckBox( trgrp, "Overrule easy settings" );
    overrule->setChecked( false );
    trgrp->setHAlignObj( xinlfld->uiObj() );
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
    applybut->attach( centeredBelow, crdgrp);

    finaliseDone.notify( mCB(this,uiSurveyInfoEditor,doFinalise) );
    survparchanged.notify( appcb );
}


void uiSurveyInfoEditor::setValues()
{
    const BinIDRange br = survinfo->range();
    const BinID bs = survinfo->step();
    StepInterval<int> inlrg( br.start.inl, br.stop.inl, bs.inl );
    StepInterval<int> crlrg( br.start.crl, br.stop.crl, bs.crl );
    inlfld->setValue( inlrg );
    crlfld->setValue( crlrg );
    if ( survinfo->zRangeUsable() )
    {
	StepInterval<double> zrg = survinfo->zRange();
	zfld->setValue( zrg );
    }

    x0fld->setValue( survinfo->b2c_.getTransform(true).a );
    xinlfld->setValue( survinfo->b2c_.getTransform(true).b );
    xcrlfld->setValue( survinfo->b2c_.getTransform(true).c );
    y0fld->setValue( survinfo->b2c_.getTransform(false).a );
    yinlfld->setValue( survinfo->b2c_.getTransform(false).b );
    ycrlfld->setValue( survinfo->b2c_.getTransform(false).c );

    Coord c[3]; BinID b[2]; int xline;
    survinfo->get3Pts( c, b, xline );
    if ( b[0].inl )
    {
	ic0fld->setValues( b[0].inl, b[0].crl );
	ic1fld->setValues( b[0].inl, xline );
	ic2fld->setValues( b[1].inl, b[1].crl );
	xy0fld->setValues( c[0].x, c[0].y );
	xy1fld->setValues( c[2].x, c[2].y );
	xy2fld->setValues( c[1].x, c[1].y );
    }
}


bool uiSurveyInfoEditor::appButPushed()
{
    if ( !setRanges() ) return false;

    if ( !overrule->isChecked() || coordset->getBoolValue() )
    {
	if ( !setCoords() ) return false;

	x0fld->setValue( survinfo->b2c_.getTransform(true).a );
	xinlfld->setValue( survinfo->b2c_.getTransform(true).b );
	xcrlfld->setValue( survinfo->b2c_.getTransform(true).c );
	y0fld->setValue( survinfo->b2c_.getTransform(false).a );
	yinlfld->setValue( survinfo->b2c_.getTransform(false).b );
	ycrlfld->setValue( survinfo->b2c_.getTransform(false).c );
	overrule->setChecked( false );
    }
    else
	if ( !setRelation() ) return false;

    survparchanged.trigger();
    return true;
}


void uiSurveyInfoEditor::doFinalise()
{
    if ( survinfo->rangeUsable() ) setValues();

    chgSetMode(0);
    ic1fld->setFldsSensible( false, 0 );
}


bool uiSurveyInfoEditor::acceptOK( CallBacker* )
{
    BufferString newdirnm = dirnmfld->text();
    if ( newdirnm == "" )
	{ uiMSG().error( "Please specify directory name." ); return false; }

    if ( !appButPushed() ) return false;

    if ( orgdirname == "" )
    {
	BufferString from( GetSoftwareDir() );
	from = File_getFullPath( GetSoftwareDir(), "data" );
	from = File_getFullPath( from, "BasicSurvey" );

	BufferString to( rootdir );
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
	survinfo->dirname = dirnmfld->text();
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
    survinfo->setName( survnmfld->text() );

    StepInterval<int> irg( inlfld->getIStepInterval() );
    StepInterval<int> crg( crlfld->getIStepInterval() );
    BinIDRange br;
    br.start.inl = irg.start; br.start.crl = crg.start;
    br.stop.inl = irg.stop;   br.stop.crl = crg.stop;
    survinfo->setRange( br );
    if ( !survinfo->rangeUsable() )
    { 
	uiMSG().error( "Please specify inline/crossline ranges" ); 
        return false; 
    }

    StepInterval<double> zrs( zfld->getDStepInterval() );
    survinfo->setZRange( zrs );

    BinID bs( irg.step, crg.step );
    if ( !bs.inl ) bs.inl = 1; if ( !bs.crl ) bs.crl = 1;
    survinfo->setStep( bs );

    return true;
}


bool uiSurveyInfoEditor::setCoords()
{    
    BinID b[2]; Coord c[3]; int xline;
    b[0].inl = ic0fld->getIntValue(0); b[0].crl = ic0fld->getIntValue(1);
    b[1].inl = ic2fld->getIntValue(0); b[1].crl = ic2fld->getIntValue(1);
    xline = ic1fld->getIntValue(1);
    c[0].x = xy0fld->getValue(0); c[0].y = xy0fld->getValue(1);
    c[1].x = xy2fld->getValue(0); c[1].y = xy2fld->getValue(1);
    c[2].x = xy1fld->getValue(0); c[2].y = xy1fld->getValue(1);
  
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


class uiIdealSurvSetup : public uiDialog
{
public:

uiIdealSurvSetup( uiParent* p, IdealConn::Type t )
    : uiDialog(p,uiDialog::Setup("Survey setup",
				 "Select cube to retrieve survey setup",
				 "0.3.8")
	    			.statusbar(true) )
{
    iddfld = new uiIdealData( this, t );
}

bool acceptOK( CallBacker* )
{
    return iddfld->fetchInput();
}

    uiIdealData* iddfld;

};


void uiSurveyInfoEditor::wsbutPush( CallBacker* )
{
    uiIdealSurvSetup dlg( this, IdealConn::guessedType() );
    if ( !dlg.go() ) return;

    const IdealConn& conn = dlg.iddfld->conn();
    BinIDSampler bs; StepInterval<double> zrg; Coord crd[3];
    if ( !conn.getSurveySetup(bs,zrg,crd) )
	{ uiMSG().error(conn.errMsg()); return; }

    survinfo->setRange( bs );
    survinfo->setZRange( zrg );
    BinID bid[2];
    bid[0].inl = bs.start.inl; bid[0].crl = bs.start.crl;
    bid[1].inl = bs.stop.inl; bid[1].crl = bs.stop.crl;
    survinfo->set3Pts( crd, bid, bs.stop.crl );
    setValues();

    survinfo->setWSProjName( SI().getWSProjName() );
    survinfo->setWSPwd( SI().getWSPwd() );
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
