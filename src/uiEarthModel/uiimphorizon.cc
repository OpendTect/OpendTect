/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.cc,v 1.40 2004-12-15 15:59:46 nanne Exp $
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhorizon.h"
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
#include "uibutton.h"
#include "uibinidsubsel.h"
#include "grid.h"
#include "scaler.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "binidselimpl.h"

#include "gridread.h"
#include "valgridtr.h"
#include "streamconn.h"


uiImportHorizon::uiImportHorizon( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Horizon",
				 "Specify horizon parameters","104.0.0"))
    , ctio(*mMkCtxtIOObj(EMHorizon))
    , emobjid(-1)
{
    infld = new uiFileInput( this, "Input Ascii file", 
	    		     uiFileInput::Setup().withexamine() );
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

    fillholesfld = new uiGenInput( this, "Try to fill small holes:",
                            BoolInpSpec() );
    fillholesfld->setValue(false);
    fillholesfld->attach( alignedBelow, scalefld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Horizon" );
    outfld->attach( alignedBelow, fillholesfld );

    displayfld = new uiCheckBox( this, "Display after import" );
    displayfld->attach( alignedBelow, outfld );
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio.ioobj; delete &ctio;
}


#define mWarnRet(s) { uiMSG().warning(s); return false; }
#define mErrRet(s) { uiMSG().error(s); return false; }
#define mErrRetUnRef(s) \
{ conn->close(); horizon->unRef(); deepErase(filenames); mErrRet(s) }

bool uiImportHorizon::handleAscii()
{
    bool doxy = xyfld->getBoolValue();

    const char* horizonnm = outfld->getInput();
    EM::EMManager& em = EM::EMM();
    emobjid = em.add( EM::EMManager::Hor, horizonnm );
    mDynamicCastGet( EM::Horizon*, horizon, em.getObject(emobjid) );
    if ( !horizon )
	mErrRet( "Cannot create horizon" );
    horizon->ref();

    BufferStringSet filenames;
    infld->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fname = filenames[idx]->buf();
	StreamConn* conn = new StreamConn( fname, Conn::Read );
	if ( conn->bad() )
	    mErrRetUnRef( "Bad connection" );
	    
	GridTranslator* trans =
	    doxy ? (GridTranslator*)CoordGridTranslator::getInstance()
		 : (GridTranslator*)BinIDGridTranslator::getInstance();

	GridReader reader( trans, conn );
	BinIDSampler* bs = 0;
	if ( !subselfld->isAll() )
	{
	    bs = new BinIDSampler;
	    HorSampling hs; subselfld->getHorSampling( hs );
	    bs->start = hs.start; bs->stop = hs.stop; bs->step = hs.step;
	}
	reader.setRange( bs );
	uiExecutor execdlg( this, reader );
	if ( !execdlg.go() ) mErrRetUnRef( "Stopped reading" );

	Grid* dskgrd = reader.grid();
	PtrMan<Grid> grid = dskgrd->cloneTrimmed();
	delete dskgrd;

	if ( !grid )
	    mErrRetUnRef( "No valid grid specified." );

	grid->ensureContainsValidZValues();
	const Scaler* scaler = scalefld->getScaler();
	if ( scaler )
	{
	    GridIter* it = grid->gridIter();
	    it->doUndef( false );
	    while ( it->valid() )
	    {
		grid->setValue( it->node(),
				scaler->scale( grid->getValue(it->node()) ) );
		it->next();
	    }
	    delete it;
	}

	PtrMan<Executor> horimp = horizon->import( *grid, idx,
					       fillholesfld->getBoolValue() );
	uiExecutor impdlg( this, *horimp );
	if ( !impdlg.go() ) 
	    mErrRetUnRef("Cannot import horizon")

	conn->close();
    }

    PtrMan<Executor> exec = horizon->geometry.saver();
    if ( !exec )
    {
	horizon->unRef();
	return false;
    }

    uiExecutor dlg( this, *exec );
    bool rv = dlg.execute();
    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDel();

    deepErase( filenames );
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

    BufferStringSet filenames;
    infld->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File_exists(fnm) )
	{
	    BufferString errmsg( "Cannot find input file:\n" );
	    errmsg += fnm;
	    deepErase( filenames );
	    mWarnRet( errmsg );
	}
    }

    deepErase( filenames );
    if ( !outfld->commitInput( true ) )
	mWarnRet( "Please select the output" )

    return true;
}


bool uiImportHorizon::doDisplay() const
{
    return displayfld->isChecked();
}


MultiID uiImportHorizon::getSelID() const
{
    if ( emobjid<0 ) return -1;

    MultiID mid = IOObjContext::getStdDirData(ctio.ctxt.stdseltype)->id;
    mid.add(emobjid);
    return mid;
}
