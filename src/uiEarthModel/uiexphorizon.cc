/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.cc,v 1.13 2003-07-29 13:03:09 nanne Exp $
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
#include "uibinidsubsel.h"
#include "survinfo.h"
#include "ctxtioobj.h"
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


bool uiExportHorizon::writeAscii( const ObjectSet<
				  TypeSet<BinIDZValue> >& bizvset )
{
    if ( bizvset.size() < 1 ) mErrRet("No values available")

    const bool doxy = typfld->getIntValue() == 0;
    const bool addzpos = zfld->getBoolValue();
    const bool dogf = typfld->getIntValue() == 2;

    const char* fname = outfld->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" );
    }

    if ( dogf )
    {
	char gfbuf[mHdr1GFLineLen+2];
	gfbuf[mHdr1GFLineLen] = '\0';
	BufferString hnm( gfnmfld->text() );
	cleanupString( hnm.buf(), NO, NO, NO );
	sprintf( gfbuf, "PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
	       		"", "", gfunfld->getBoolValue() ? "m " : "ft" );
	int sz = hnm.size(); if ( sz > 17 ) sz = 17;
	memcpy( gfbuf+8, hnm.buf(), sz );
	hnm = gfcommfld->text();
	sz = hnm.size(); if ( sz > 45 ) sz = 45;
	memcpy( gfbuf+35, hnm.buf(), sz );
	*sdo.ostrm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << endl;
    }

    Coord crd;
    for ( int iset=0; iset<bizvset.size(); iset++ )
    {
	const TypeSet<BinIDZValue>& bizvs = *bizvset[iset];

	for ( int idx=0; idx<bizvs.size(); idx++ )
	{
	    const BinIDZValue& bizv = bizvs[idx];
	    if ( mIsUndefined(bizv.value) ) continue;

	    if ( doxy || dogf )
		crd = SI().transform( bizv.binid );
	    if ( dogf )
		writeGF( *sdo.ostrm, bizv, crd, iset );
	    else
	    {
		if ( doxy )
		    *sdo.ostrm << crd.x << '\t' << crd.y;
		else
		    *sdo.ostrm << bizv.binid.inl << '\t' << bizv.binid.crl;

		if ( addzpos )
		{
		    float z = bizv.z;
		    if ( SI().zIsTime() ) z *= 1000;
		    *sdo.ostrm << '\t' << z;
		}
		*sdo.ostrm << '\t' << bizvs[idx].value << '\n';
	    }
	}

	if ( !dogf && iset < bizvset.size() - 1 )
	    // Should we write some kind of marker when more than 1 set?
	    ;
    }

    if ( dogf ) *sdo.ostrm << "EOD";
    sdo.close();
    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( File_exists(outfld->fileName()) && 
			!uiMSG().askGoOn( "File exists. Continue?" ) )
	return false;

    return true;
}


void uiExportHorizon::typChg( CallBacker* )
{
    const bool isgf = typfld->getIntValue() == 2;
    gfgrp->display( isgf );
    zfld->display( !isgf );
}
