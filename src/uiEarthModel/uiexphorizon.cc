/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.cc,v 1.1 2002-08-08 10:33:12 nanne Exp $
________________________________________________________________________

-*/

#include "uiexphorizon.h"

#include "strmdata.h"
#include "strmprov.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "filegen.h"
#include "uimsg.h"
#include "uibinidsubsel.h"
#include "uilistbox.h"
#include "survinfo.h"
#include "geompos.h"


uiExportHorizon::uiExportHorizon( uiParent* p, 
				  const ObjectSet<BufferString>& strs )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format",0))

{
    inbox = new uiLabeledListBox( this, "Available horizons" );
    inbox->box()->addItems( strs );

    xyfld = new uiGenInput( this, "Positions in:",
                            BoolInpSpec("X/Y","Inl/Crl") );
    xyfld->attach( alignedBelow, inbox );

    zfld = new uiGenInput( this, "Include depth", BoolInpSpec() );
    zfld->setValue( false );
    zfld->attach( alignedBelow, xyfld );

    outfld = new uiFileInput( this, "Output Ascii file", "", false );
    BufferString datadirnm( GetDataDir() );
    BufferString dirnm = File_getFullPath( datadirnm, "Surfaces" );
    if ( !File_exists( dirnm ) )
	dirnm = File_getFullPath( datadirnm, "Grids" );
    outfld->setDefaultSelectionDir( dirnm );
    outfld->attach( alignedBelow, zfld );
}


uiExportHorizon::~uiExportHorizon()
{
}


#define mWarnRet(s) { uiMSG().warning(s); return false; }
#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportHorizon::writeAscii( const TypeSet<BinIDValue>& bids,
				  const TypeSet<float>& values )
{
    bool doxy = xyfld->getBoolValue();
    bool addzpos = zfld->getBoolValue();

    const char* fname = outfld->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" );
    }

    for ( int idx=0; idx< bids.size(); idx++ )
    {
	Geometry::Pos pos;
	pos.z = bids[idx].value;
	BinID bid = bids[idx].binid;
	if ( doxy )
	{
	    Coord crd = SI().transform( bid );
	    pos.x = crd.x; pos.y = crd.y;
	}
	else
	{
	    pos.x = bid.inl; pos.y = bid.crl;
	}

	*sdo.ostrm << pos.x << '\t' << pos.y << '\t';
	if ( addzpos ) *sdo.ostrm << pos.z << '\t';
	*sdo.ostrm << values[idx] << '\n';
    }

    sdo.close();
    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( File_exists(outfld->fileName()) && 
			!uiMSG().askGoOn( "File exists. Continue?" ) )
	return false;

    if ( inbox->box()->currentItem() < 0 )
	mWarnRet( "Please select input" );

    return true;
}


const char* uiExportHorizon::selectedItem()
{
    return inbox->box()->getText();
}

