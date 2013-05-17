/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiconvpos.h"
#include "survinfo.h"
#include "strmprov.h"
#include "oddirs.h"
#include "uitoolbutton.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uimsg.h"
#include <iostream>

#define mMaxLineBuf 32000
static BufferString lastinpfile;
static BufferString lastoutfile;


uiConvertPos::uiConvertPos( uiParent* p, const SurveyInfo& si, bool mod )
	: uiDialog(p, uiDialog::Setup("Position conversion",
		   "Coordinates vs Inline/Crossline","0.3.7").modal(mod))
	, survinfo(si)
{
    ismanfld = new uiGenInput( this, "Conversion",
	           BoolInpSpec(true,"Manual","File") );
    ismanfld->valuechanged.notify( mCB(this,uiConvertPos,selChg) );

    mangrp = new uiGroup( this, "Manual group" );
    uiGroup* inlcrlgrp = new uiGroup( mangrp, "InlCrl group" );
    const Interval<int> intv( -mUdf(int), mUdf(int) );
    inlfld = new uiGenInput( inlcrlgrp, "In-line", 
		IntInpSpec(0,intv).setName("Inl-field") );
    inlfld->setElemSzPol( uiObject::Small );
    crlfld = new uiGenInput( inlcrlgrp, "Cross-line", 
		IntInpSpec(0,intv).setName("Crl-field") );
    crlfld->setElemSzPol( uiObject::Small );
    crlfld->attach( alignedBelow, inlfld );

    uiGroup* xygrp = new uiGroup( mangrp, "XY group" );
    xfld = new uiGenInput( xygrp, "X-coordinate",
			   DoubleInpSpec().setName("X-field") );
    xfld->setElemSzPol( uiObject::Small );
    yfld = new uiGenInput( xygrp, "Y-coordinate", 
	    		   DoubleInpSpec().setName("Y-field") );
    yfld->setElemSzPol( uiObject::Small );
    yfld->attach( alignedBelow, xfld );

    uiGroup* butgrp = new uiGroup( mangrp, "Buttons" );
    uiToolButton* dobinidbut = new uiToolButton( butgrp,
			uiToolButton::LeftArrow, "Convert (X,Y) to Inl/Crl",
			mCB(this,uiConvertPos,getBinID) );
    uiToolButton* docoordbut = new uiToolButton( butgrp,
	    		uiToolButton::RightArrow, "Convert Inl/Crl to (X,Y)",
			mCB(this,uiConvertPos,getCoord) );
    docoordbut->attach( rightTo, dobinidbut );
    butgrp->attach( centeredRightOf, inlcrlgrp );
    xygrp->attach( centeredRightOf, butgrp );

    mangrp->setHAlignObj( inlfld );
    mangrp->attach( alignedBelow, ismanfld );

    filegrp = new uiGroup( this, "File group" );
    uiFileInput::Setup fipsetup( lastinpfile );
    fipsetup.forread(true).withexamine(true)
	    .examstyle(uiFileInput::Setup::Table).defseldir(GetDataDir());
    inpfilefld = new uiFileInput( filegrp, "Input file", fipsetup );

    fipsetup.fnm = lastoutfile;
    fipsetup.forread(false).withexamine(false);
    outfilefld = new uiFileInput( filegrp, "Output file", fipsetup );
    outfilefld->attach( alignedBelow, inpfilefld );
    isxy2bidfld = new uiGenInput( filegrp, "Type",
	           BoolInpSpec(true,"X/Y to I/C","I/C to X/Y") );
    isxy2bidfld->attach( alignedBelow, outfilefld );
    uiPushButton* pb = new uiPushButton( filegrp, "Go",
	    			mCB(this,uiConvertPos,convFile), true );
    pb->attach( alignedBelow, isxy2bidfld );
    filegrp->setHAlignObj( inpfilefld );
    filegrp->attach( alignedBelow, ismanfld );

    setCtrlStyle( LeaveOnly );
    postFinalise().notify( mCB(this,uiConvertPos,selChg) );
}


void uiConvertPos::selChg( CallBacker* )
{
    const bool isman = ismanfld->getBoolValue();
    mangrp->display( isman );
    filegrp->display( !isman );
}


void uiConvertPos::getCoord( CallBacker* )
{
    BinID binid( inlfld->getIntValue(), crlfld->getIntValue() );
    if ( binid == BinID::udf() )
    {
	uiMSG().error( "Cannot convert this position" );
	xfld->setText( "" ); yfld->setText( "" );
	return;
    }

    Coord coord( survinfo.transform( binid ) );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
}


void uiConvertPos::getBinID( CallBacker* )
{
    Coord coord( xfld->getdValue(), yfld->getdValue() );
    if ( coord == Coord::udf() )
    {
	uiMSG().error( "Cannot convert this position" );
	inlfld->setText( "" ); crlfld->setText( "" );
	return;
    }

    BinID binid( survinfo.transform( coord ) );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiConvertPos::convFile( CallBacker* )
{
    char buf[255];
    const BufferString inpfnm = inpfilefld->fileName();
    StreamData sdin = StreamProvider(inpfnm).makeIStream();
    if ( !sdin.usable() )
	mErrRet("Input file is not readable" );

    const BufferString outfnm = outfilefld->fileName();
    StreamData sdout = StreamProvider(outfnm).makeOStream();
    if ( !sdout.usable() )
	{ sdin.close(); mErrRet("Cannot open output file" ); }

    lastinpfile = inpfnm; lastoutfile = outfnm;

    char linebuf[mMaxLineBuf]; Coord c;
    const bool xy2ic = isxy2bidfld->getBoolValue();
    int nrln = 0;
    while ( *sdin.istrm )
    {
	*sdin.istrm >> c.x;
	if ( sdin.istrm->eof() ) break;
	*sdin.istrm >> c.y;
	sdin.istrm->getline( linebuf, mMaxLineBuf );
	if ( sdin.istrm->bad() ) break;
	if ( xy2ic )
	{
	    BinID bid( SI().transform(c) );
	    *sdout.ostrm << bid.inl << ' ' << bid.crl << linebuf << '\n';
	}
	else
	{
	    BinID bid( mNINT32(c.x), mNINT32(c.y) );
	    c = SI().transform( bid );
	    getStringFromDouble( 0, c.x, buf );
	    *sdout.ostrm << buf << ' ';
	    getStringFromDouble( 0, c.y, buf );
	    *sdout.ostrm << buf << linebuf << '\n';
	}
	nrln++;
    }

    sdin.close(); sdout.close();
    getStringFromInt(nrln,buf);
    uiMSG().message( "Total number of converted lines: ", buf );
}


