/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.cc,v 1.2 2002-08-12 14:20:50 nanne Exp $
________________________________________________________________________

-*/

#include "uiexphorizon.h"

#include "strmdata.h"
#include "strmprov.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "filegen.h"
#include "uimsg.h"
#include "uibinidsubsel.h"
#include "uilistbox.h"
#include "survinfo.h"
#include "geompos.h"


uiExportHorizon::uiExportHorizon( uiParent* p, 
				  const ObjectSet<BufferString>& strs,
				  const TypeSet<int>& horids_, 
				  const ObjectSet<BufferString>& attribs_ )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format",0))
	, selhorid(-1)
	, horids(horids_)
	, attribs(attribs_)

{
    inbox = new uiLabeledListBox( this, "Available horizons" );
    inbox->box()->addItems( strs );
    inbox->box()->selectionChanged.notify( mCB(this,uiExportHorizon,selChg) );

    attrlbl = new uiLabel( this, "" );
    attrlbl->setHSzPol( uiObject::medium );
    attrlbl->attach( alignedBelow, inbox );
    uiLabel* lbltxt = new uiLabel( this, "Calculated attribute: " );
    lbltxt->attach( leftOf, attrlbl );

    xyfld = new uiGenInput( this, "Positions in:",
                            BoolInpSpec("X/Y","Inl/Crl") );
    xyfld->attach( alignedBelow, attrlbl );

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

    selChg( 0 );
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


void uiExportHorizon::selChg( CallBacker* )
{
    int selitmnr = inbox->box()->currentItem();
    selhorid = horids[selitmnr];
    attrlbl->setText( *attribs[selitmnr] );
}
