/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.cc,v 1.15 2003-08-14 09:46:29 nanne Exp $
________________________________________________________________________

-*/

#include "uiexphorizon.h"

#include "strmdata.h"
#include "strmprov.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "filegen.h"
#include "uimsg.h"
#include "uiiosurface.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "emmanager.h"
#include "emhorizontransl.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "uiexecutor.h"
#include "ptrman.h"
#include "geomgridsurface.h"

#include <stdio.h>

static const char* exptyps[] = { "X/Y", "Inl/Crl", "IESX (3d_ci7m)", 0 };


uiExportHorizon::uiExportHorizon( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format","104.0.1"))
{
    infld = new uiSurfaceSel( this );

    typfld = new uiGenInput( this, "Output type", StringListInpSpec(exptyps) );
    typfld->attach( alignedBelow, infld );
    typfld->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );

    BufferString lbltxt( "Include Z (" );
    lbltxt += SI().zIsTime() ? "Time)" : "Depth)";
    zfld = new uiGenInput( this, lbltxt, BoolInpSpec() );
    zfld->setValue( false );
    zfld->attach( alignedBelow, typfld );

    gfgrp = new uiGroup( this, "GF things" );
    gfnmfld = new uiGenInput( gfgrp, "Horizon name in file" );
    gfcommfld = new uiGenInput( gfgrp, "Comment" );
    gfcommfld->attach( alignedBelow, gfnmfld );
    gfunfld = new uiGenInput( gfgrp, "Coordinates are in",
	    			BoolInpSpec("m","ft") );
    gfunfld->attach( alignedBelow, gfcommfld );
    gfgrp->setHAlignObj( gfnmfld );
    gfgrp->attach( alignedBelow, typfld );

    outfld = new uiFileInput( this, "Output Ascii file", "", false );
    outfld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    outfld->attach( alignedBelow, gfgrp );

    typChg( 0 );
}


uiExportHorizon::~uiExportHorizon()
{
}


#define mWarnRet(s) { uiMSG().warning(s); return false; }
#define mErrRet(s) { uiMSG().error(s); return false; }
#define mHdr1GFLineLen 102
#define mDataGFLineLen 148

static void initGF( ostream& strm, const char* hornm, bool inmeter, 
		    const char* comment )
{
    char gfbuf[mHdr1GFLineLen+2];
    gfbuf[mHdr1GFLineLen] = '\0';
    BufferString hnm( hornm );
    cleanupString( hnm.buf(), NO, NO, NO );
    sprintf( gfbuf, "PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
		    "", "", inmeter ? "m " : "ft" );
    int sz = hnm.size(); if ( sz > 17 ) sz = 17;
    memcpy( gfbuf+8, hnm.buf(), sz );
    hnm = comment;
    sz = hnm.size(); if ( sz > 45 ) sz = 45;
    memcpy( gfbuf+35, hnm.buf(), sz );
    strm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << endl;
}


static void writeGF( ostream& strm, const BinIDZValue& bizv,
		     const Coord& crd, int segid )
{
    static char buf[mDataGFLineLen+2];
    const float crl = bizv.binid.crl;
    const float val = mIsUndefined(bizv.value) ? 3.4028235E+38 : bizv.value;
    sprintf( buf, "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	     crd.x, crd.y, segid, 14, bizv.z*1000, crl, crl, bizv.binid.crl,
	     val, bizv.binid.inl, "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


bool uiExportHorizon::writeAscii()
{
    const bool doxy = typfld->getIntValue() == 0;
    const bool addzpos = zfld->getBoolValue();
    const bool dogf = typfld->getIntValue() == 2;

    BufferString basename = outfld->fileName();

    PtrMan<EM::EMObject> obj = EM::EMM().getTempObj( EM::EMManager::Hor );
    mDynamicCastGet(EM::Horizon*,hor,obj.ptr())
    if ( !hor ) return false;

    dgbEMHorizonTranslator tr;
    tr.startRead( *infld->selIOObj() );
    EM::SurfaceIODataSelection& sels = tr.selections();
    infld->getSelection( sels );
    PtrMan<Executor> exec = tr.reader( *hor );
    if ( !exec ) mErrRet( "Cannot read selected horizon" );

    uiExecutor dlg( this, *exec );
    if ( !dlg.go() ) return false;

    bool saveauxdata = sels.selvalues.size();
    TypeSet<int>& patches = sels.selpatches;
    for ( int idx=0; idx<patches.size(); idx++ )
    {
	int patchidx = patches[idx];
	BufferString fname( basename ); 
	if ( patchidx )
	{ fname += "^"; fname += idx; }

	StreamData sdo = StreamProvider( fname ).makeOStream();
	if ( !sdo.usable() )
	{
	    sdo.close();
	    mErrRet( "Cannot open output file" );
	}

	if ( dogf )
	    initGF( *sdo.ostrm, gfnmfld->text(), gfunfld->getBoolValue(), 
		    gfcommfld->text() );

	const EM::PatchID patchid = hor->patchID( patchidx );
	const Geometry::GridSurface* gridsurf = hor->getSurface( patchid );
	EM::PosID posid( infld->selIOObj()->key(), patchid );
	const int nrnodes = gridsurf->size();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Geometry::PosID geomposid = gridsurf->getPosID(idy);
	    const Coord3 crd = gridsurf->getPos( geomposid );
	    const BinID bid = SI().transform(crd);
	    float auxvalue = mUndefValue;
	    if ( saveauxdata && hor->nrAuxData() )
	    {
		const RowCol emrc( bid.inl, bid.crl );
		const EM::SubID subid = hor->rowCol2SubID( emrc );
		posid.setSubID( subid );
		auxvalue = hor->getAuxDataVal(0,posid);
	    }
	    
	    if ( dogf )
	    {
		BinIDZValue bzv( bid, crd.z, auxvalue );
		writeGF( *sdo.ostrm, bzv, crd, idy );
	    }
	    else
	    {
		if ( doxy )
		    *sdo.ostrm << crd.x << '\t' << crd.y;
		else
		    *sdo.ostrm << bid.inl << '\t' << bid.crl;

		if ( addzpos )
		{
		    float z = crd.z;
		    if ( SI().zIsTime() ) z *= 1000;
		    *sdo.ostrm << '\t' << z;
		}

		if ( saveauxdata )
		    *sdo.ostrm << '\t' << auxvalue;

		*sdo.ostrm << '\n';
	    }

	}

	if ( dogf ) *sdo.ostrm << "EOD";
	sdo.close();
    }

    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( !strcmp(outfld->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File_exists(outfld->fileName()) && 
			!uiMSG().askGoOn( "File exists. Continue?" ) )
	return false;

    return writeAscii();
}


void uiExportHorizon::typChg( CallBacker* )
{
    const bool isgf = typfld->getIntValue() == 2;
    gfgrp->display( isgf );
    zfld->display( !isgf );
}
