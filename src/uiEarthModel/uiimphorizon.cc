/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.cc,v 1.25 2003-07-30 17:01:03 bert Exp $
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
				 "Specify horizon parameters","104.0.0"))
    , ctio(*new CtxtIOObj(EMHorizonTranslator::ioContext()))
{
    infld = new uiFileInput( this, "Input Ascii file");
    infld->setSelectMode( uiFileDialog::ExistingFiles );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );

    xyfld = new uiGenInput( this, "Positions in:",
                            BoolInpSpec("X/Y","Inl/Crl") );
    xyfld->attach( alignedBelow, infld );

    subselfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup()
	    			   .withz(false).withstep(true) );
    subselfld->attach( alignedBelow, xyfld );

    BufferString scalelbl( SI().zIsTime() ? "Z " : "Depth " );
    scalelbl += "scaling";
    scalefld = new uiScaler( this, scalelbl, true );
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
#define mErrRetUnRef(s) { horizon->unRef(); mErrRet(s) }

bool uiImportHorizon::handleAscii()
{
    bool doxy = xyfld->getBoolValue();

    const char* horizonnm = outfld->getInput();
    EM::EMManager& em = EM::EMM();
    MultiID key = em.add( EM::EMManager::Hor, horizonnm );
    mDynamicCastGet( EM::Horizon*, horizon, em.getObject( key ) );
    if ( !horizon )
	mErrRet( "Cannot create horizon" );
    horizon->ref();

    ObjectSet<BufferString> filenames;
    infld->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fname = filenames[idx]->buf();
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

	if ( !grid )
	    mErrRet( "No valid grid specified." );

	Scaler* scaler = scalefld->getScaler();

	if ( !scaler )
	    grid->ensureContainsValidZValues();
	else
	{
	    GridScaler grdsc( grid, scaler );
	    uiExecutor scdlg( this, grdsc );
	    scdlg.go();
	}

	PtrMan<Executor> horimp = horizon->import( *grid, idx );
	uiExecutor impdlg( this, *horimp );
	if ( !impdlg.go() ) 
	    mErrRetUnRef("Cannot import horizon")

    }

    PtrMan<Executor> exec = horizon->saver();
    uiExecutor dlg( this, *exec );
    bool rv = dlg.execute();
    horizon->unRef();
    return rv;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && handleAscii();
    return ret;
}


bool uiImportHorizon::checkInpFlds()
{
    if ( ! *infld->fileName() )
	mWarnRet( "Please select input file(s)" )

    ObjectSet<BufferString> filenames;
    infld->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File_exists(fnm) )
	{
	    BufferString errmsg( "Cannot find input file:\n" );
	    errmsg += fnm;
	    mWarnRet( errmsg );
	}
    }

    if ( !outfld->commitInput( true ) )
	mWarnRet( "Please select the output" )

    return true;
}

