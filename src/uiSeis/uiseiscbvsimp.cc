/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Jun 2002
 RCS:		$Id: uiseiscbvsimp.cc,v 1.46 2008-05-13 13:07:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseiscbvsimp.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"
#include "keystrs.h"
#include "executor.h"

#include "uimsg.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "uitaskrunner.h"

uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p )
	: uiDialog(p,Setup("Import CBVS cube",
		    	   "Specify import parameters",
			   "103.0.1"))
	, inctio_(*mMkCtxtIOObj(SeisTrc))
	, outctio_(*mMkCtxtIOObj(SeisTrc))
    	, modefld(0)
{
    init( false );
    modeSel(0);
}


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p, const IOObj* ioobj )
	: uiDialog(p,Setup("Copy cube data",
		    	   "Specify copy parameters",
			   "103.1.1"))
	, inctio_(*mMkCtxtIOObj(SeisTrc))
	, outctio_(*mMkCtxtIOObj(SeisTrc))
    	, modefld(0)
{
    if ( ioobj ) inctio_.ioobj = ioobj->clone();
    init( true );
    oinpSel(0);
}


void uiSeisImpCBVS::init( bool fromioobj )
{
    finpfld = 0; modefld = typefld = 0; oinpfld = 0;
    setTitleText( fromioobj ? "Specify transfer parameters"
	    		    : "Create CBVS cube definition" );
    tmpid_ = "100010."; tmpid_ += IOObj::tmpID;

    if ( fromioobj )
    {
	inctio_.ctxt.forread = true;
	inctio_.ctxt.trglobexpr = "CBVS";
	oinpfld = new uiSeisSel( this, inctio_, uiSeisSel::Setup(Seis::Vol) );
	oinpfld->selectiondone.notify( mCB(this,uiSeisImpCBVS,oinpSel) );
    }
    else
    {
	BufferString seldir = IOObjContext::getDataDirName(IOObjContext::Seis);
	finpfld = new uiFileInput( this, "(First) CBVS file name",
	       			   uiFileInput::Setup()
				   .filter("CBVS (*.cbvs)") );
	finpfld->setDefaultSelectionDir( seldir );
	finpfld->valuechanged.notify( mCB(this,uiSeisImpCBVS,finpSel) );

	StringListInpSpec spec;
	spec.addString( "Input data cube" );
	spec.addString( "Generated attribute cube" );
	spec.addString( "Steering cube" );
	typefld = new uiGenInput( this, "Cube type", spec );
	typefld->attach( alignedBelow, finpfld );
	typefld->valuechanged.notify( mCB(this,uiSeisImpCBVS,typeChg) );

	modefld = new uiGenInput( this, "Import mode",
			  BoolInpSpec(false,"Copy the data","Use in-place") );
	modefld->attach( alignedBelow, typefld );
	modefld->valuechanged.notify( mCB(this,uiSeisImpCBVS,modeSel) );
    }

    uiSeisTransfer::Setup sts( Seis::Vol );
    sts.withnullfill(fromioobj).withstep(true).onlyrange(false)
				.fornewentry(true);
    transffld = new uiSeisTransfer( this, sts );
    transffld->attach( alignedBelow,
	    		modefld ? (uiGroup*)modefld : (uiGroup*)oinpfld );

    outctio_.ctxt.forread = false;
    outctio_.ctxt.trglobexpr = "CBVS";
    IOM().to( outctio_.ctxt.getSelKey() );
    outfld = new uiSeisSel( this, outctio_, uiSeisSel::Setup(Seis::Vol) );
    outfld->attach( alignedBelow, transffld );
}


uiSeisImpCBVS::~uiSeisImpCBVS()
{
    delete outctio_.ioobj; delete &outctio_;
    delete inctio_.ioobj; delete &inctio_;
}


IOObj* uiSeisImpCBVS::getfInpIOObj( const char* inp ) const
{
    IOStream* iostrm = new IOStream( "_tmp", tmpid_ );
    iostrm->setGroup( outctio_.ctxt.trgroup->userName() );
    iostrm->setTranslator( outctio_.ctxt.trglobexpr );
    iostrm->setFileName( inp );
    return iostrm;
}


void uiSeisImpCBVS::modeSel( CallBacker* )
{
    if ( modefld )
	transffld->display( modefld->getBoolValue() );
}


void uiSeisImpCBVS::typeChg( CallBacker* )
{
    bool issteer = typefld ? typefld->getIntValue() == 2 : false;
    if ( oinpfld )
    {
	oinpfld->commitInput(false);
	if ( !inctio_.ioobj ) return;
	const char* res = inctio_.ioobj->pars().find( "Type" );
	issteer = res && *res == 'S';
    }

    transffld->setSteering( issteer );
}


void uiSeisImpCBVS::oinpSel( CallBacker* cb )
{
    if ( inctio_.ioobj )
	transffld->updateFrom( *inctio_.ioobj );
    typeChg( cb );
}


void uiSeisImpCBVS::finpSel( CallBacker* )
{
    const char* out = outfld->getInput();
    if ( *out ) return;
    BufferString inp = finpfld->text();
    if ( !*(const char*)inp ) return;

    if ( !File_isEmpty(inp) )
    {
	PtrMan<IOObj> ioobj = getfInpIOObj( inp );
	transffld->updateFrom( *ioobj );
    }

    inp = FilePath( inp ).fileName();
    if ( !*(const char*)inp ) return;

    // convert underscores to spaces
    char* ptr = inp.buf();
    while ( *ptr )
    {
	if ( *ptr == '_' ) *ptr = ' ';
	ptr++;
    }

    // remove .cbvs extension
    ptr--;
    if ( *ptr == 's' && *(ptr-1) == 'v' && *(ptr-2) == 'b'
     && *(ptr-3) == 'c' && *(ptr-4) == '.' )
	*(ptr-4) = '\0';

    outfld->setInputText( inp );
}


#define rmTmpIOObj() IOM().permRemove( MultiID(tmpid_.buf()) );

#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSeisImpCBVS::acceptOK( CallBacker* )
{
    if ( !outfld->commitInput(true) )
    {
	uiMSG().error( "Please choose a valid name for the output data" );
	return false;
    }

    const bool dolink = modefld && !modefld->getBoolValue();
    if ( oinpfld )
    {
	if ( !oinpfld->commitInput(false) )
	{
	    uiMSG().error( "Please select an input cube" );
	    return false;
	}
	outctio_.ioobj->pars() = inctio_.ioobj->pars();
    }
    else
    {
	const char* fname = finpfld->text();
	if ( !fname || !*fname )
	{
	    uiMSG().error( "Please select the input filename" );
	    return false;
	}
	const int seltyp = typefld->getIntValue();
	if ( !seltyp )
	    outctio_.ioobj->pars().removeWithKey( "Type" );
	else
	    outctio_.ioobj->pars().set( sKey::Type, seltyp == 1
		    			? sKey::Attribute : sKey::Steering );

	outctio_.ioobj->setTranslator( "CBVS" );
	if ( !dolink )
	{
	    inctio_.setObj( getfInpIOObj(fname) );
	    IOM().commitChanges( *inctio_.ioobj );
	}
	else
	{
	    mDynamicCastGet(IOStream*,iostrm,outctio_.ioobj);
	    iostrm->setFileName( fname );
	}
    }

    IOM().commitChanges( *outctio_.ioobj );
    if ( dolink )
	return true;

    uiSeisIOObjInfo ioobjinfo( *outctio_.ioobj, true );
    if ( !ioobjinfo.checkSpaceLeft(transffld->spaceInfo()) )
	{ rmTmpIOObj(); return false; }

    const char* titl = oinpfld ? "Copying seismic data"
			       : "Importing CBVS seismic cube";
    const char* attrnm = oinpfld ? oinpfld->attrNm() : "";
    PtrMan<Executor> stp = transffld->getTrcProc( *inctio_.ioobj,
	    			*outctio_.ioobj, titl, "Loading data",
				attrnm );
    if ( !stp )
	{ rmTmpIOObj(); return false; }

    uiTaskRunner dlg( this );
    const bool rv = dlg.execute(*stp) && !ioobjinfo.is2D() &&
		    ioobjinfo.provideUserInfo();

    rmTmpIOObj();
    return rv;
}
