/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.cc,v 1.5 2001-09-28 12:06:32 bert Exp $
________________________________________________________________________

-*/

#include "uisurvinfoed.h"
#include "errh.h"
#include "filegen.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uisurvmap.h"
#include "survinfo.h"


uiSurveyInfoEditor::uiSurveyInfoEditor( uiParent* p, SurveyInfo* si, 
					uiSurveyMap* map )
	: uiDialog(p,"Survey setup")
	, rootdir( getenv("dGB_DATA") )
	, dirnmch_(0)
	, survinfo(si)
	, survmap(map)

{
    setTitleText( "Specify survey parameters" );

    orgdirname = survinfo->dirname;
    survnm = survinfo->name();

    survnmfld = new uiGenInput( this, "Survey name", StringInpSpec( survnm ) );
    dirnmfld = new uiGenInput( this, "Directory name", 
			       StringInpSpec( orgdirname ) );
    BufferString nm = "( "; nm += rootdir; nm += "/ )";
    uiLabel* dirnm = new uiLabel( this, nm );
    dirnmfld->attach( alignedBelow, survnmfld );
    dirnm->attach( rightOf, dirnmfld );

    uiGroup* rangegrp = new uiGroup( this, "Survey ranges" );
    inlfld = new uiGenInput( rangegrp, "In-line range",
			     IntInpIntervalSpec(true) );
    crlfld = new uiGenInput( rangegrp, "Cross-line range",
			     IntInpIntervalSpec(true) );
    zfld = new uiGenInput( rangegrp, "Time range (s)",
			   DoubleInpIntervalSpec(true) );
    crlfld->attach( alignedBelow, inlfld );
    zfld->attach( alignedBelow, crlfld );

    coordset = new uiGenInput( this, "Coordinate settings", BoolInpSpec( "Easy",
			       "Advanced" ) );
    coordset->changed.notify( mCB(this,uiSurveyInfoEditor,chgSetMode) );
    coordgrp = new uiGroup( this, "Coordinate settings" );
    ic0fld = new uiGenInput( coordgrp, "First In-line/Cross-line",
	    		     IntInpSpec(), IntInpSpec() );
    ic0fld->changed.notify( mCB(this,uiSurveyInfoEditor,setInl1Fld) );
    ic1fld = new uiGenInput( coordgrp, "Cross-line on above in-line",
			     IntInpSpec(), IntInpSpec() );
    ic2fld = new uiGenInput( coordgrp, 
			     "In-line/Cross-line not on above in-line",
			     IntInpSpec(), IntInpSpec() );
    xy0fld = new uiGenInput( coordgrp, "= (X,Y)", DoubleInpSpec(), 
			     DoubleInpSpec() ); 
    xy1fld = new uiGenInput( coordgrp, "= (X,Y)", DoubleInpSpec(), 
			     DoubleInpSpec() ); 
    xy2fld = new uiGenInput( coordgrp, "= (X,Y)", DoubleInpSpec(), 
			     DoubleInpSpec() );
    ic1fld->attach( alignedBelow, ic0fld );
    ic2fld->attach( alignedBelow, ic1fld );
    xy0fld->attach( rightOf, ic0fld );
    xy1fld->attach( rightOf, ic1fld );
    xy2fld->attach( rightOf, ic2fld );
    applybut = new uiPushButton( this, "Apply" ); 
    applybut->activated.notify( mCB(this,uiSurveyInfoEditor,appButPushed) );
    applybut->attach( centeredBelow, coordgrp);

    trgrp = new uiGroup( this, "I/C to X/Y transformation" );
    x0fld = new uiGenInput ( trgrp, "X = ", DoubleInpSpec() );
    xinlfld = new uiGenInput ( trgrp, "+ in-line *", DoubleInpSpec() );
    xcrlfld = new uiGenInput ( trgrp, "+ cross-line *", DoubleInpSpec() );
    y0fld = new uiGenInput ( trgrp, "Y = ", DoubleInpSpec() );
    yinlfld = new uiGenInput ( trgrp, "+ in-line *", DoubleInpSpec() );
    ycrlfld = new uiGenInput ( trgrp, "+ cross-line *", DoubleInpSpec() );
    overrule= new uiCheckBox( trgrp, "Overrule easy settings" );
    overrule->setChecked( false );
    xinlfld->attach( rightOf, x0fld );
    xcrlfld->attach( rightOf, xinlfld );
    y0fld->attach( alignedBelow, x0fld );
    yinlfld->attach( rightOf, y0fld );
    ycrlfld->attach( rightOf, yinlfld );
    overrule->attach( alignedBelow, ycrlfld );

    uiSeparator* horsep1 = new uiSeparator( this );
    uiSeparator* horsep2 = new uiSeparator( this );
    uiLabel* labelrg = new uiLabel( this, "Survey ranges:" );
    rangegrp->setHAlignObj( inlfld->uiObj() );
    coordgrp->setHAlignObj( ic0fld->uiObj() );
    trgrp->setHAlignObj( xinlfld->uiObj() );
    horsep1->attach( stretchedBelow, dirnmfld );
    labelrg->attach( leftBorder );
    labelrg->attach( ensureBelow, horsep1 );
    rangegrp->attach( alignedBelow, dirnmfld ); 
    rangegrp->attach( ensureBelow, labelrg ); 
    horsep2->attach( stretchedBelow, rangegrp );
    coordset->attach( leftBorder );
    coordset->attach( ensureBelow, horsep2 );
    coordgrp->attach( alignedBelow, rangegrp );
    coordgrp->attach( ensureBelow, coordset );
    trgrp->attach( alignedBelow, rangegrp );
    trgrp->attach( ensureBelow, coordset );
    trgrp->hide();

    if ( survinfo->rangeUsable() ) setValues();

    finalising.notify( mCB(this,uiSurveyInfoEditor,doFinalise) );

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

    if ( !overrule->isChecked() )
    {
	if ( !setCoords() ) return false;

	x0fld->setValue( survinfo->b2c_.getTransform(true).a );
	xinlfld->setValue( survinfo->b2c_.getTransform(true).b );
	xcrlfld->setValue( survinfo->b2c_.getTransform(true).c );
	y0fld->setValue( survinfo->b2c_.getTransform(false).a );
	yinlfld->setValue( survinfo->b2c_.getTransform(false).b );
	ycrlfld->setValue( survinfo->b2c_.getTransform(false).c );
    }
    else
	if ( !setRelation() ) return false;

    survmap->drawMap( survinfo );
    return true;
}


void uiSurveyInfoEditor::doFinalise()
{
    ic1fld->setFldsSensible( false, 0 );
}


bool uiSurveyInfoEditor::acceptOK( CallBacker* )
{
    BufferString newdirnm = dirnmfld->text();
    if ( newdirnm == "" )
	{ ErrMsg( "Please specify directory name." ); return false; }

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
	    ErrMsg( errmsg ); 
	    return false;
	}

	if ( !File_copy( from, to, YES ) )
	{
	    ErrMsg( "Cannot create proper new survey directory" );
	    return false;
	}

	File_makeWritable( to, YES, YES );
	survinfo->dirname = dirnmfld->text();
    }
    else
        if ( orgdirname != newdirnm ) dirnmch_ = true;

    if ( !survinfo->write( rootdir ) )
        ErrMsg( "Failed to write survey info.\nNo changes committed." );
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
        { ErrMsg( "Please specify inline/crossline ranges" ); return false; }

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
    if ( msg ) { ErrMsg( msg ); return false; }

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
        ErrMsg( "The transformation is not valid." );
        return false;
    }

    survinfo->b2c_.setTransforms( xtr, ytr );
    return true;
}


void uiSurveyInfoEditor::chgSetMode( CallBacker* )
{
    coordgrp->display( coordset->getBoolValue() );
    trgrp->display( !coordset->getBoolValue() );
}


void uiSurveyInfoEditor::setInl1Fld( CallBacker* )
{
    ic1fld->setText( ic0fld->text(0), 0 );
}
