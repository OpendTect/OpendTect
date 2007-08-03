/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.24 2007-08-03 09:50:11 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiimppickset.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tabledef.h"
#include "filegen.h"
#include "pickset.h"
#include "picksettr.h"

#include <math.h>

static const char* zoptions[] =
{
    "Input file",
    "Constant Z",
    0
};


uiImpExpPickSet::uiImpExpPickSet( uiParent* p, bool imp )
    : uiDialog(p,uiDialog::Setup(imp ? "Import Pickset" : "Export PickSet",
				 "Specify pickset parameters",
				 imp ? "105.0.1" : "105.0.2"))
    , ctio_(*mMkCtxtIOObj(PickSet))
    , import_(imp)
    , fd_(*PickSetAscIO::getDesc(true,true))
    , xyfld_(0)
{
    BufferString label( import_ ? "Input " : "Output " );
    label += "Ascii file";
    filefld_ = new uiFileInput( this, label, uiFileInput::Setup()
					    .withexamine(import_)
					    .forread(import_) );
    filefld_->setDefaultSelectionDir( 
			    IOObjContext::getDataDirName(IOObjContext::Loc) );

    ctio_.ctxt.forread = !import_;
    ctio_.ctxt.maychdir = false;
    label = import_ ? "Output " : "Input "; label += "PickSet";
    objfld_ = new uiIOObjSel( this, ctio_, label );

    if ( import_ )
    {
	xyfld_ = new uiGenInput( this, "Positions in:",
				BoolInpSpec(true,"X/Y","Inl/Crl") );
	xyfld_->valuechanged.notify( mCB(this,uiImpExpPickSet,formatSel) );
	xyfld_->attach( alignedBelow, filefld_ );

	zfld_ = new uiLabeledComboBox( this, "Get Z values from" );
	zfld_->box()->addItems( zoptions );
	zfld_->box()->selectionChanged.notify( mCB(this,uiImpExpPickSet,
		    		formatSel) );
	zfld_->attach( alignedBelow, xyfld_ );

	constzfld_ = new uiGenInput( this, "Specify Constatnt Z value",
				FloatInpSpec(0) );
	constzfld_->attach( alignedBelow, zfld_ );
	constzfld_->display( zfld_->box()->currentItem() == 1 );

	uiSeparator* sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, constzfld_ );

	dataselfld_ = new uiTableImpDataSel( this, fd_, "100.0.0" );
	dataselfld_->attach( alignedBelow, constzfld_ );
	dataselfld_->attach( ensureBelow, sep );

	sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, dataselfld_ );

	objfld_->attach( alignedBelow, sep );
	objfld_->attach( ensureBelow, sep );
    }
    else
	filefld_->attach( alignedBelow, objfld_ );
}


uiImpExpPickSet::~uiImpExpPickSet()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiImpExpPickSet::formatSel( CallBacker* cb )
{
    const bool isxy = xyfld_->getBoolValue();
    const int zchoice = zfld_->box()->currentItem(); 
    const bool iszreq = zchoice == 0;
    constzfld_->display( zchoice == 1 );
    Table::FormatDesc& fd = dataselfld_->desc();
    fd.bodyinfos_[0]->setName( isxy ? "X Coordinate" : "Inl No." );
    fd.bodyinfos_[1]->setName( isxy ? "Y Coordinate" : "Crl No." );
    if ( iszreq )
    {
	if ( fd.bodyinfos_.size() == 2 )
	    fd.bodyinfos_ += new Table::TargetInfo( "Z Values", FloatInpSpec(),
		    		 Table::Required, PropertyRef::surveyZType() );
    }
    else
    {
	if ( fd.bodyinfos_.size() == 3 )
	{
	    Table::TargetInfo* ti = fd.bodyinfos_.remove( 2 );
	    delete ti;
	}
    }
}

#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiImpExpPickSet::doImport()
{
    const char* fname = filefld_->fileName();
    StreamData sdi = StreamProvider( fname ).makeIStream();
    if ( !sdi.usable() ) 
    { 
	sdi.close();
	mErrRet( "Could not open input file" )
    }

    const char* psnm = objfld_->getInput();
    Pick::Set ps( psnm );
    bool isxy = xyfld_ ? xyfld_->getBoolValue() : true;
    const int zchoice = zfld_->box()->currentItem();
    const float constz = zchoice==1 ? constzfld_->getfValue() : 0;
    ps.disp_.color_ = Color::DgbColor;
    PickSetAscIO aio( fd_ );
    aio.get( *sdi.istrm, ps, isxy, zchoice==0, constz );
    sdi.close();

    BufferString errmsg;
    if ( !PickSetTranslator::store(ps,ctio_.ioobj,errmsg) )
	mErrRet(errmsg);

    return true;
}


bool uiImpExpPickSet::doExport()
{
    Pick::Set ps;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(ps,ctio_.ioobj,errmsg) )
	mErrRet(errmsg);

    const char* fname = filefld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() ) 
    { 
	sdo.close();
	mErrRet( "Could not open output file" )
    }

    BufferString buf;
    for ( int locidx=0; locidx<ps.size(); locidx++ )
    {
	ps[locidx].toString( buf );
	*sdo.ostrm << buf.buf() << '\n';
    }

    *sdo.ostrm << '\n';
    sdo.close();
    return true;
}


bool uiImpExpPickSet::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;
    bool ret = import_ ? doImport() : doExport();
    return ret;
}


bool uiImpExpPickSet::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( import_ && !File_exists(filenm) )
	mErrRet( "Please select input file" );

    if ( !import_ && filenm.isEmpty() )
	mErrRet( "Please select output file" );

    if ( !objfld_->commitInput( true ) )
	mErrRet( "Please select PickSet" );

    return true;
}
