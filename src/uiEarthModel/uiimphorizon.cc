/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.cc,v 1.13 2002-06-13 14:44:26 bert Exp $
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "emmanager.h"
#include "emhorizon.h"
#include "emhorizontransl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "uiioobjsel.h"
#include "strmdata.h"
#include "strmprov.h"
#include "uiexecutor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "filegen.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uibinidsubsel.h"
#include "gridmods.h"
#include "scaler.h"
#include "survinfo.h"

#include "gridread.h"
#include "valgridtr.h"
#include "streamconn.h"


uiImportHorizon::uiImportHorizon( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Import Horizon",
				     "Specify horizon parameters",0))
	, ctio(*new CtxtIOObj(EarthModelHorizonTranslator::ioContext()))
{
    infld = new uiFileInput( this, "Input Ascii file");
    BufferString dirnm( GetDataDir() );
    dirnm = File_getFullPath( dirnm, "Surfaces" );
    infld->setDefaultSelectionDir( dirnm );

    xyfld = new uiGenInput( this, "Positions in:",
                             BoolInpSpec("X/Y","Inl/Crl") );
    xyfld->attach( alignedBelow, infld );

    subselfld = new uiBinIDSubSel( this, false, false, false );
    subselfld->attach( alignedBelow, xyfld );

    scalefld = new uiScaler( this, "Value scaling", true );
    scalefld->attach( alignedBelow, subselfld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Horizon" );
    outfld->attach( alignedBelow, scalefld );
}


uiImportHorizon::~uiImportHorizon()
{
    delete &ctio;
}


#define mWarnRet(s) { uiMSG().warning(s); return false; }
#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImportHorizon::handleAscii()
{
    bool doxy = xyfld->getBoolValue();

    const char* fname = infld->fileName();
    StreamConn* conn = new StreamConn( fname, Conn::Read );
    if ( conn->bad() )
	mErrRet( "Bad connection" );
	
    GridTranslator* trans =
	doxy ? (GridTranslator*) new CoordGridTranslator
	     : (GridTranslator*) new BinIDGridTranslator;

    GridReader reader( trans, conn );
    BinIDRange* bidrg = subselfld->isAll() ? 0 : subselfld->getRange();
    reader.setRange( bidrg );
    uiExecutor execdlg( this, reader );
    if ( !execdlg.go() ) return false;

    Grid* dskgrd = reader.grid();
    PtrMan<Grid> grid = dskgrd->cloneTrimmed();
    delete dskgrd;

    Scaler* scaler = scalefld->getScaler();

    // Auto-detect millisecond grids
    if ( !scaler )
    {
	GridNode lastgn( grid->nrCols()-1, grid->nrRows()-1 );
	const float maxval = grid->partValue( FeatureSpec::Max,
					      GridNode(0,0), lastgn );
	if ( maxval > 2 * SI().zRange().stop )
	{
	    if ( SI().zRange().includes( maxval * .001 ) )
		scaler = new LinScaler( 0, 0.001 );
	}
    }

    if ( scaler )
    {
	GridScaler* grdsc = new GridScaler( grid, scaler );
	uiExecutor scdlg( this, *grdsc );
	scdlg.go();
    }

    const char* horizonnm = outfld->getInput();
    EarthModel::EMManager& em = EarthModel::EMM();

    MultiID key = em.add( EarthModel::EMManager::Hor, horizonnm );
    mDynamicCastGet( EarthModel::Horizon*, horizon, em.getObject( key ) );
    if ( !horizon )
	mErrRet( "Cannot create horizon" );

    if ( !horizon->import( *grid ) )
	mErrRet( "Cannot import horizon" );

    BufferString msg;
    PtrMan<Executor> exec =
	EarthModelHorizonTranslator::writer( *horizon, ctio.ioobj, msg );
    uiExecutor dlg( this, *exec );
    dlg.go();
    if ( msg && *msg )
	mErrRet( msg )

    return true;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && handleAscii();
    return ret;
}


bool uiImportHorizon::checkInpFlds()
{
    if ( !File_exists(infld->fileName()) )
	mWarnRet( "Please select input,\nor file does not exist" );

    if ( !outfld->commitInput( true ) )
	mWarnRet( "Please select output" );

    return true;
}

