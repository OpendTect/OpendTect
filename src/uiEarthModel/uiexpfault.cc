/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id: uiexpfault.cc,v 1.6 2008-11-24 10:59:18 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiexpfault.h"

#include "ctxtioobj.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
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

#define mGet( tp, fss, f3d ) \
    !strcmp(tp,EMFaultStickSetTranslatorGroup::keyword) ? fss : f3d

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D) )
#define mGetTitle(tp) \
    mGet( tp, "Export FaultStickSet", "Export Fault" )

uiExportFault::uiExportFault( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup(mGetTitle(typ),
				 "Specify output format","104.1.1"))
    , ctio_(mGetCtio(typ))
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


static int nrSticks( EM::EMObject* emobj, EM::SectionID sid )
{
    mDynamicCastGet(EM::Fault3D*,f3d,emobj)
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj)
    return f3d ? f3d->geometry().nrSticks( sid )
               : fss->geometry().nrSticks( sid );
}

static int nrKnots( EM::EMObject* emobj, EM::SectionID sid, int stickidx )
{
    mDynamicCastGet(EM::Fault3D*,f3d,emobj)
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj)
    return f3d ? f3d->geometry().nrKnots( sid, stickidx )
               : fss->geometry().nrKnots( sid, stickidx );
}

static Coord3 getCoord( EM::EMObject* emobj, EM::SectionID sid, int stickidx,
			int knotidx )
{
    mDynamicCastGet(EM::Fault3D*,f3d,emobj)
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj)
    const RowCol rc( stickidx, knotidx );
    return f3d ? f3d->geometry().sectionGeometry(sid)->getKnot( rc )
	       : fss->geometry().sectionGeometry(sid)->getKnot( rc );
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportFault::writeAscii()
{
    const IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) mErrRet("Cannot find fault in database");

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot add fault to EarthModel")

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Fault3D*,f3d,emobj.ptr())
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj.ptr())
    if ( !f3d && !fss ) return false;

    PtrMan<Executor> loader = emobj->loader();
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

    BufferString str;
    const EM::SectionID sectionid = emobj->sectionID( 0 );
    const int nrsticks = nrSticks( emobj.ptr(), sectionid );
    for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
    {
	const int nrknots = nrKnots( emobj.ptr(), sectionid, stickidx );
	for ( int knotidx=0; knotidx<nrknots; knotidx++ )
	{
	    const Coord3 crd = getCoord( emobj.ptr(), sectionid,
		    			 stickidx, knotidx );
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
