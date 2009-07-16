/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiexphorizon.cc,v 1.61 2009-07-16 12:51:51 cvsbert Exp $";

#include "uiexphorizon.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "filegen.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include <stdio.h>

static const char* exptyps[] = { "X/Y", "Inl/Crl", "IESX (3d_ci7m)", 0 };


uiExportHorizon::uiExportHorizon( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format","104.0.1"))
{
    setCtrlStyle( DoAndStay );

    infld_ = new uiSurfaceRead( this,
	    uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::keyword())
	    .withsubsel(true) );
    infld_->inpChange.notify( mCB(this,uiExportHorizon,inpSel) );
    infld_->attrSelChange.notify( mCB(this,uiExportHorizon,attrSel) );

    typfld_ = new uiGenInput( this, "Output type", StringListInpSpec(exptyps) );
    typfld_->attach( alignedBelow, infld_ );
    typfld_->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );

    BufferString lbltxt( "Include Z (" );
    lbltxt += SI().zIsTime() ? "Time)" : "Depth)";
    zfld_ = new uiGenInput( this, lbltxt, BoolInpSpec(true) );
    zfld_->setValue( false );
    zfld_->attach( alignedBelow, typfld_ );
    zfld_->valuechanged.notify( mCB(this,uiExportHorizon,addZChg) );
    bool setchk = true;
    if ( SI().zIsTime() )
	zbox_ = new uiCheckBox( this, "Msec" );
    else
    {
	zbox_ = new uiCheckBox( this, "Feet" );
	setchk = SI().depthsInFeetByDefault();
    }
    zbox_->setChecked( setchk );
    zbox_->attach( rightTo, zfld_ );

    udffld_ = new uiGenInput( this, "Undefined value",
	    		     StringInpSpec(sKey::FloatUdf) );
    udffld_->attach( alignedBelow, zfld_ );

    gfgrp_ = new uiGroup( this, "GF things" );
    gfnmfld_ = new uiGenInput( gfgrp_, "Horizon name in file" );
    gfcommfld_ = new uiGenInput( gfgrp_, "Comment" );
    gfcommfld_->attach( alignedBelow, gfnmfld_ );
    gfgrp_->setHAlignObj( gfnmfld_ );
    gfgrp_->attach( alignedBelow, typfld_ );

    outfld_ = new uiFileInput( this, "Output Ascii file",
	    		      uiFileInput::Setup().forread(false) );
    outfld_->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    outfld_->attach( alignedBelow, gfgrp_ );

    typChg( 0 );
    inpSel( 0 );
}


uiExportHorizon::~uiExportHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mHdr1GFLineLen 102
#define mDataGFLineLen 148

static void initGF( std::ostream& strm, const char* hornm,
		    const char* comment )
{
    char gfbuf[mHdr1GFLineLen+2];
    gfbuf[mHdr1GFLineLen] = '\0';
    BufferString hnm( hornm );
    cleanupString( hnm.buf(), mC_False, mC_False, mC_False );
    sprintf( gfbuf, "PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
		    "", "", SI().xyInFeet() ? "ft" : "m " );
    int sz = hnm.size(); if ( sz > 17 ) sz = 17;
    memcpy( gfbuf+8, hnm.buf(), sz );
    hnm = comment;
    sz = hnm.size(); if ( sz > 45 ) sz = 45;
    memcpy( gfbuf+35, hnm.buf(), sz );
    strm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << std::endl;
}


#define mGFUndefValue 3.4028235E+38

static void writeGF( std::ostream& strm, const BinID& bid, float z, float val,
		     const Coord& crd, int segid )
{
    static char buf[mDataGFLineLen+2];
    const float crl = bid.crl;
    const float gfval = mIsUdf(val) ? mGFUndefValue : val;
    const float zfac = SI().zIsTime() ? 1000
		    : (SI().xyInFeet() ? mToFeetFactor : 1);
    const float depth = mIsUdf(z) ? mGFUndefValue : z * zfac;
    sprintf( buf, "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	     crd.x, crd.y, segid, 14, depth, crl, crl, bid.crl, gfval, bid.inl,
	     "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


bool uiExportHorizon::writeAscii()
{
    const bool doxy = typfld_->getIntValue() == 0;
    const bool addzpos = zfld_->getBoolValue();
    const bool dogf = typfld_->getIntValue() == 2;
    BufferString udfstr = udffld_->text();
    if ( udfstr.isEmpty() ) udfstr = sKey::FloatUdf;

    BufferString basename = outfld_->fileName();

    const IOObj* ioobj = infld_->selIOObj();
    if ( !ioobj ) mErrRet("Cannot find horizon object");

    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    em.getSurfaceData( ioobj->key(), sd );
    EM::SurfaceIODataSelection sels( sd );
    infld_->getSelection( sels );
    sels.selvalues.erase();

    RefMan<EM::EMObject> emobj = em.createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot create horizon")

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
    PtrMan<Executor> loader = hor->geometry().loader( &sels );
    if ( !loader ) mErrRet("Cannot read horizon")

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) ) return false;

    infld_->getSelection( sels );
    if ( dogf && sels.selvalues.size() > 1 &&
	    !uiMSG().askContinue("Only the first selected attribute will be used\n"
			     "Do you wish to continue?") )
	return false;

    if ( !sels.selvalues.isEmpty() )
    {
	ExecutorGroup exgrp( "Reading aux data" );
	for ( int idx=0; idx<sels.selvalues.size(); idx++ )
	    exgrp.add( hor->auxdata.auxDataLoader(sels.selvalues[idx]) );

	uiTaskRunner datatask( this );
	if ( !datatask.execute(exgrp) ) return false;
    }

    MouseCursorChanger cursorlock( MouseCursor::Wait );

    const float zfac = !addzpos || !zbox_->isChecked() ? 1
			: (SI().zIsTime() ? 1000 : mToFeetFactor);
    const int nrattribs = hor->auxdata.nrAuxData();
    TypeSet<int>& sections = sels.selsections;
    const bool writemultiple = sections.size() > 1;
    for ( int sidx=0; sidx<sections.size(); sidx++ )
    {
	const int sectionidx = sections[sidx];
	BufferString fname( basename ); 
	if ( writemultiple )
	{
	    FilePath fp( fname );
	    BufferString ext( fp.extension() );
	    if ( ext.isEmpty() )
		{ fname += "_"; fname += sidx; }
	    else
	    {
		fp.setExtension( 0 );
		BufferString fnm = fp.fileName();
		fnm += "_"; fname += sidx;
		fp.setFileName( fnm );
		fp.setExtension( ext );
		fname = fp.fullPath();
	    }
	}

	StreamData sdo = StreamProvider( fname ).makeOStream();
	if ( !sdo.usable() )
	{
	    sdo.close();
	    mErrRet( "Cannot open output file" );
	}

	if ( dogf )
	    initGF( *sdo.ostrm, gfnmfld_->text(), gfcommfld_->text() );

	const EM::SectionID sectionid = hor->sectionID( sectionidx );
	PtrMan<EM::EMObjectIterator> it = hor->createIterator( sectionid );
	BufferString str;
	while ( true )
	{
	    const EM::PosID posid = it->next();
	    if ( posid.objectID()==-1 )
		break;

	    const Coord3 crd = hor->getPos( posid );

	    if ( dogf )
	    {
		const BinID bid = SI().transform( crd );
		const float auxvalue = nrattribs > 0
		    ? hor->auxdata.getAuxDataVal(0,posid) : mUdf(float);
		writeGF( *sdo.ostrm, bid, crd.z, auxvalue, crd, sidx );
		continue;
	    }

	    if ( !doxy )
	    {
		const BinID bid = SI().transform( crd );
		*sdo.ostrm << bid.inl << '\t' << bid.crl;
	    }
	    else
	    {
		// ostreams print doubles awfully
		str.setEmpty();
		str += crd.x; str += "\t"; str += crd.y;
		*sdo.ostrm << str;
	    }

	    if ( addzpos )
	    {
		if ( mIsUdf(crd.z) ) 
		    *sdo.ostrm << '\t' << udfstr;
		else
		{
		    str = "\t"; str += zfac * crd.z;
		    *sdo.ostrm << str;
		}
	    }

	    for ( int idx=0; idx<nrattribs; idx++ )
	    {
		const float auxvalue = hor->auxdata.getAuxDataVal( idx, posid );
		if ( mIsUdf(auxvalue) )
		    *sdo.ostrm << '\t' << udfstr;
		else
		{
		    str = "\t"; str += auxvalue;
		    *sdo.ostrm << str;
		}
	    }

	    *sdo.ostrm << '\n';
	}

	if ( dogf ) *sdo.ostrm << "EOD";
	sdo.close();
    }

    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( !strcmp(outfld_->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File_exists(outfld_->fileName()) && 
			!uiMSG().askOverwrite("Output file exists. Overwrite?") )
	return false;

    writeAscii();
    return false;
}


void uiExportHorizon::typChg( CallBacker* cb )
{
    const bool isgf = typfld_->getIntValue() == 2;
    gfgrp_->display( isgf );
    zfld_->display( !isgf );

    attrSel( cb );
    addZChg( cb );
}


void uiExportHorizon::inpSel( CallBacker* )
{
    const IOObj* ioobj = infld_->selIOObj();
    if ( ioobj )
	gfnmfld_->setText( ioobj->name() );
}


void uiExportHorizon::addZChg( CallBacker* )
{
    zbox_->display( typfld_->getIntValue() != 2 && zfld_->getBoolValue() );
}


void uiExportHorizon::attrSel( CallBacker* )
{
    const bool isgf = typfld_->getIntValue() == 2;
    udffld_->display( !isgf && infld_->haveAttrSel() );
}
