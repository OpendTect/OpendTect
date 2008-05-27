/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id: uiexpfault.cc,v 1.4 2008-05-27 12:11:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiexpfault.h"

#include "ctxtioobj.h"
#include "emfault.h"
#include "emfaulttransl.h"
#include "emmanager.h"
#include "executor.h"
#include "filegen.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include <stdio.h>

uiExportFault::uiExportFault( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Fault",
				 "Specify output format","104.1.1"))
    , ctio_(*mMkCtxtIOObj(EMFault))
{
    infld_ = new uiIOObjSel( this, ctio_, "Input Fault" );

    coordfld_ = new uiGenInput( this, "Write coordinates as",
				BoolInpSpec(true,"X/Y","Inl/Crl") );
    coordfld_->attach( alignedBelow, infld_ );

    stickfld_ = new uiCheckBox( this, "stick index" );
    stickfld_->setChecked( true );
    stickfld_->attach( alignedBelow, coordfld_ );
    nodefld_ = new uiCheckBox( this, "node index" );
    nodefld_->setChecked( false );
    nodefld_->attach( rightTo, stickfld_ );
    uiLabel* lbl = new uiLabel( this, "Write", stickfld_ );

    outfld_ = new uiFileInput( this, "Output Ascii file",
	    		       uiFileInput::Setup().forread(false) );
    outfld_->setDefaultSelectionDir(
		IOObjContext::getDataDirName(IOObjContext::Surf) );
    outfld_->attach( alignedBelow, stickfld_ );
}


uiExportFault::~uiExportFault()
{
    delete ctio_.ioobj; delete &ctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportFault::writeAscii()
{
    const IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) mErrRet("Cannot find fault in database");

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot add fault to EarthModel")

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Fault*,fault,emobj.ptr())
    PtrMan<Executor> loader = fault->geometry().loader();
    if ( !loader ) mErrRet("Cannot read fault")

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) ) return false;

    const BufferString fname = outfld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" );
    }

    const bool doxy = coordfld_->getBoolValue();
    const bool inclstickidx = stickfld_->isChecked();
    const bool inclknotidx = nodefld_->isChecked();

    const EM::SectionID sectionid = fault->sectionID( 0 );
    const Geometry::FaultStickSurface* fltgeom =
	fault->geometry().sectionGeometry( sectionid );

    BufferString str;
    const int nrsticks = fault->geometry().nrSticks( sectionid );
    for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
    {
	const int nrknots = fault->geometry().nrKnots( sectionid, stickidx );
	for ( int knotidx=0; knotidx<nrknots; knotidx++ )
	{
	    const Coord3 crd = fltgeom->getKnot( RowCol(stickidx,knotidx) );
	    if ( !crd.isDefined() )
		continue;

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

	    *sdo.ostrm << '\t' << crd.z*SI().zFactor();

	    if ( inclstickidx )
		*sdo.ostrm << '\t' << stickidx;
	    if ( inclknotidx )
		*sdo.ostrm << '\t' << knotidx;

	    *sdo.ostrm << '\n';

	}
    }

    sdo.close();
    return true;
}


bool uiExportFault::acceptOK( CallBacker* )
{
    if ( !strcmp(outfld_->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File_exists(outfld_->fileName()) && 
			!uiMSG().askGoOn("Output file exists. Continue?") )
	return false;

    return writeAscii();
}
