/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.cc,v 1.33 2004-12-17 12:31:09 bert Exp $
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
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "uiexecutor.h"
#include "ptrman.h"
#include "geommeshsurface.h"

#include <stdio.h>

static const char* exptyps[] = { "X/Y", "Inl/Crl", "IESX (3d_ci7m)", 0 };


uiExportHorizon::uiExportHorizon( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format","104.0.1"))
{
    infld = new uiSurfaceRead( this, true );
    infld->attrSelChange.notify( mCB(this,uiExportHorizon,attrSel) );

    typfld = new uiGenInput( this, "Output type", StringListInpSpec(exptyps) );
    typfld->attach( alignedBelow, infld );
    typfld->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );

    BufferString lbltxt( "Include Z (" );
    lbltxt += SI().zIsTime() ? "Time)" : "Depth)";
    zfld = new uiGenInput( this, lbltxt, BoolInpSpec() );
    zfld->setValue( false );
    zfld->attach( alignedBelow, typfld );

    udffld = new uiGenInput( this, "Undefined value",
	    		     StringInpSpec(sUndefValue) );
    udffld->attach( alignedBelow, zfld );

    gfgrp = new uiGroup( this, "GF things" );
    gfnmfld = new uiGenInput( gfgrp, "Horizon name in file" );
    gfcommfld = new uiGenInput( gfgrp, "Comment" );
    gfcommfld->attach( alignedBelow, gfnmfld );
    gfunfld = new uiGenInput( gfgrp, "Coordinates are in",
	    			BoolInpSpec("m","ft") );
    gfunfld->attach( alignedBelow, gfcommfld );
    gfgrp->setHAlignObj( gfnmfld );
    gfgrp->attach( alignedBelow, typfld );

    outfld = new uiFileInput( this, "Output Ascii file",
	    		      uiFileInput::Setup().forread(false) );
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

static void initGF( std::ostream& strm, const char* hornm, bool inmeter, 
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
    strm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << std::endl;
}


#define mGFUndefValue 3.4028235E+38

static void writeGF( std::ostream& strm, const BinID& bid, float z, float val,
		     const Coord& crd, int segid )
{
    static char buf[mDataGFLineLen+2];
    const float crl = bid.crl;
    const float gfval = mIsUndefined(val) ? mGFUndefValue : val;
    const float zfac = SI().zIsTime() ? 1000 : 1;
    const float depth = mIsUndefined(z) ? mGFUndefValue : z * zfac;
    sprintf( buf, "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	     crd.x, crd.y, segid, 14, depth, crl, crl, bid.crl, gfval, bid.inl,
	     "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


bool uiExportHorizon::writeAscii()
{
    const bool doxy = typfld->getIntValue() == 0;
    const bool addzpos = zfld->getBoolValue();
    const bool dogf = typfld->getIntValue() == 2;
    BufferString udfstr = udffld->text();
    if ( udfstr == "" ) udfstr = sUndefValue;

    BufferString basename = outfld->fileName();

    PtrMan<EM::EMObject> obj = EM::EMM().getTempObj( EM::EMManager::Hor );
    mDynamicCastGet(EM::Horizon*,hor,obj.ptr())
    if ( !hor ) return false;

    IOObj* ioobj = infld->selIOObj();
    PtrMan<EMSurfaceTranslator> tr =
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
	mErrRet( tr ? tr->errMsg() : "Cannot find translator" );

    EM::SurfaceIODataSelection& sels = tr->selections();
    infld->getSelection( sels );
    if ( dogf && sels.selvalues.size() > 1 &&
	    !uiMSG().askGoOn("Only the first selected attribute will be used\n"
			     "Do you wish to continue?") )
	return false;

    PtrMan<Executor> exec = tr->reader( *hor );
    if ( !exec ) mErrRet( "Cannot read selected horizon" );

    uiExecutor dlg( this, *exec );
    if ( !dlg.go() ) return false;

    const float zfac = SI().zIsTime() ? 1000 : 1;
    const int nrattribs = hor->auxdata.nrAuxData();
    TypeSet<int>& sections = sels.selsections;
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	const int sectionidx = sections[idx];
	BufferString fname( basename ); 
	if ( sectionidx )
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

	const EM::SectionID sectionid = hor->geometry.sectionID( sectionidx );
	const Geometry::MeshSurface* meshsurf =
	    				hor->geometry.getSurface( sectionid );
	EM::PosID posid(
		EM::EMManager::multiID2ObjectID(infld->selIOObj()->key()),
		sectionid );
	const int nrnodes = meshsurf->size();
	BufferString str;
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Geometry::PosID geomposid = meshsurf->getPosID(idy);
	    const Coord3 crd = meshsurf->getPos( geomposid );
	    const BinID bid = SI().transform(crd);
	    float auxvalue = mUndefValue;
	    if ( nrattribs )
	    {
		const RowCol emrc( bid.inl, bid.crl );
		const EM::SubID subid = hor->geometry.rowCol2SubID( emrc );
		posid.setSubID( subid );
		auxvalue = hor->auxdata.getAuxDataVal(0,posid);
	    }
	    
	    if ( dogf )
	    {
		writeGF( *sdo.ostrm, bid, crd.z, auxvalue, crd, idx );
		continue;
	    }

	    if ( !doxy )
		*sdo.ostrm << bid.inl << '\t' << bid.crl;
	    else
	    {
		// ostreams print doubles awfully
		str = "";
		str += crd.x; str += "\t"; str += crd.y;
		*sdo.ostrm << str;
	    }

	    if ( addzpos )
	    {
		if ( mIsUndefined(crd.z) ) 
		    *sdo.ostrm << '\t' << udfstr;
		else
		    *sdo.ostrm << '\t' << zfac * crd.z;
	    }

	    for ( int idx=0; idx<nrattribs; idx++ )
	    {
		auxvalue = hor->auxdata.getAuxDataVal(idx,posid);
		if ( mIsUndefined(auxvalue) )
		    *sdo.ostrm << '\t' << udfstr;
		else
		    *sdo.ostrm << '\t' << auxvalue;
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
    if ( !strcmp(outfld->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File_exists(outfld->fileName()) && 
			!uiMSG().askGoOn( "File exists. Continue?" ) )
	return false;

    return writeAscii();
}


void uiExportHorizon::typChg( CallBacker* cb )
{
    const bool isgf = typfld->getIntValue() == 2;
    gfgrp->display( isgf );
    zfld->display( !isgf );
    attrSel( cb );
}


void uiExportHorizon::attrSel( CallBacker* )
{
    const bool isgf = typfld->getIntValue() == 2;
    udffld->display( !isgf && infld->haveAttrSel() );
}
