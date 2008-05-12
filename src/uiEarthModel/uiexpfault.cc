/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id: uiexpfault.cc,v 1.1 2008-05-12 03:57:24 cvsnanne Exp $
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
#include "uifileinput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include <stdio.h>

static const char* exptyps[] = { "X/Y", "Inl/Crl", 0 };

uiExportFault::uiExportFault( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Export Fault",
				     "Specify output format","104.0.1"))
{
    infld_ = new uiSurfaceRead( this,
	    uiSurfaceRead::Setup(EMFaultTranslatorGroup::keyword)
	    .withattribfld(false).withsubsel(false).withsectionfld(false) );

    typfld_ = new uiGenInput( this, "Output type", StringListInpSpec(exptyps) );
    typfld_->attach( alignedBelow, infld_ );
    typfld_->valuechanged.notify( mCB(this,uiExportFault,typeChg) );

    outfld_ = new uiFileInput( this, "Output Ascii file",
	    		       uiFileInput::Setup().forread(false) );
    outfld_->setDefaultSelectionDir(
		IOObjContext::getDataDirName(IOObjContext::Surf) );
    outfld_->attach( alignedBelow, typfld_ );

    typeChg( 0 );
}


uiExportFault::~uiExportFault()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiExportFault::writeAscii()
{
    const bool doxy = typfld_->getIntValue() == 0;

    const IOObj* ioobj = infld_->selIOObj();
    if ( !ioobj ) mErrRet("Cannot find horizon object");

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot create horizon")

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Fault*,fault,emobj.ptr())
    PtrMan<Executor> loader = fault->geometry().loader();
    if ( !loader ) mErrRet("Cannot read horizon")

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) ) return false;

    BufferString fname = outfld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" );
    }

    const EM::SectionID sectionid = fault->sectionID( 0 );
    PtrMan<EM::EMObjectIterator> it = fault->createIterator( sectionid );
    BufferString str;
    while ( true )
    {
	const EM::PosID posid = it->next();
	if ( posid.objectID()==-1 )
	    break;

	const Coord3 crd = fault->getPos( posid );

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

	*sdo.ostrm << '\t' << crd.z << '\n';
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


void uiExportFault::typeChg( CallBacker* cb )
{
}
