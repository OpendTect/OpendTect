/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiimppickset.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipickpartserv.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "file.h"
#include "pickset.h"
#include "picksettr.h"

#include <math.h>

static const char* zoptions[] =
{
    "Input file",
    "Constant Z",
    "Horizon",
    0
};


uiImpExpPickSet::uiImpExpPickSet( uiPickPartServer* p, bool imp )
    : uiDialog(p->parent(),uiDialog::Setup(imp ? "Import Pickset/Polygon"
			: "Export PickSet/Polygon", mNoDlgTitle,
			imp ? "105.0.1" : "105.0.2"))
    , serv_(p)
    , import_(imp)
    , fd_(*PickSetAscIO::getDesc(true))
    , zfld_(0)
    , constzfld_(0)
    , dataselfld_(0)
{
    setCtrlStyle( DoAndStay );

    BufferString label( import_ ? "Input " : "Output " );
    label += "Ascii file";
    filefld_ = new uiFileInput( this, label, uiFileInput::Setup()
					    .withexamine(import_)
					    .forread(import_) );

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.forread = !import_;
    label = import_ ? "Output " : "Input "; label += "PickSet/Polygon";
    objfld_ = new uiIOObjSel( this, ctxt, label );

    if ( import_ )
    {
	zfld_ = new uiLabeledComboBox( this, "Get Z values from" );
	zfld_->box()->addItems( zoptions );
	zfld_->box()->selectionChanged.notify( mCB(this,uiImpExpPickSet,
		    		formatSel) );
	zfld_->attach( alignedBelow, filefld_ );

	BufferString constzlbl = "Specify constant Z value";
	constzlbl += SI().getZUnitString();
	constzfld_ = new uiGenInput( this, constzlbl, FloatInpSpec(0) );
	constzfld_->attach( alignedBelow, zfld_ );
	constzfld_->display( zfld_->box()->currentItem() == 1 );

	horinpfld_ = new uiLabeledComboBox( this, "Select Horizon" );
	serv_->fetchHors( false );
	const ObjectSet<SurfaceInfo> hinfos = serv_->horInfos();
	for ( int idx=0; idx<hinfos.size(); idx++ )
	    horinpfld_->box()->addItem( hinfos[idx]->name );
	horinpfld_->attach( alignedBelow, zfld_ );
	horinpfld_->display( zfld_->box()->currentItem() == 2 );

	uiSeparator* sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, constzfld_ );

	dataselfld_ = new uiTableImpDataSel( this, fd_, "105.0.5" );
	dataselfld_->attach( alignedBelow, constzfld_ );
	dataselfld_->attach( ensureBelow, sep );

	sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, dataselfld_ );

	objfld_->attach( alignedBelow, constzfld_ );
	objfld_->attach( ensureBelow, sep );

	colorfld_ = new uiColorInput( this,
	       		           uiColorInput::Setup(getRandStdDrawColor()).
	       			   lbltxt("Color") );
	colorfld_->attach( alignedBelow, objfld_ );

	polyfld_ = new uiCheckBox( this, "Import as Polygon" );
	polyfld_->attach( rightTo, colorfld_ );
    }
    else
	filefld_->attach( alignedBelow, objfld_ );
}


void uiImpExpPickSet::formatSel( CallBacker* cb )
{
    const int zchoice = zfld_->box()->currentItem(); 
    const bool iszreq = zchoice == 0;
    constzfld_->display( zchoice == 1 );
    horinpfld_->display( zchoice == 2 );
    PickSetAscIO::updateDesc( fd_, iszreq );
    dataselfld_->updateSummary();
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
    const int zchoice = zfld_->box()->currentItem();
    float constz = zchoice==1 ? constzfld_->getfValue() : 0;
    if ( SI().zIsTime() ) constz /= 1000;

    ps.disp_.color_ = colorfld_->color();
    PickSetAscIO aio( fd_ );
    aio.get( *sdi.istrm, ps, zchoice==0, constz );
    sdi.close();

    if ( zchoice == 2 )
    {
	serv_->fillZValsFrmHor( &ps, horinpfld_->box()->currentItem() );
    }

    const IOObj* objfldioobj = objfld_->ioobj();
    if ( !objfldioobj ) return false;
    PtrMan<IOObj> ioobj = objfldioobj->clone();
    const bool ispolygon = polyfld_->isChecked();
    if ( ispolygon )
    {
	ps.disp_.connect_ = Pick::Set::Disp::Close;
	ioobj->pars().set( sKey::Type, sKey::Polygon );
    }
    else
    {
	ps.disp_.connect_ = Pick::Set::Disp::None;
	ioobj->pars().set( sKey::Type, PickSetTranslatorGroup::sKeyPickSet() );
    }

    IOM().commitChanges( *ioobj );
    BufferString errmsg;
    if ( !PickSetTranslator::store(ps,ioobj,errmsg) )
	mErrRet(errmsg);

    return true;
}


bool uiImpExpPickSet::doExport()
{
    const IOObj* objfldioobj = objfld_->ioobj();
    if ( !objfldioobj ) return false;

    PtrMan<IOObj> ioobj = objfldioobj->clone();
    BufferString errmsg; Pick::Set ps;
    if ( !PickSetTranslator::retrieve(ps,ioobj,true, errmsg) )
	mErrRet(errmsg);

    const char* fname = filefld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() ) 
    { 
	sdo.close();
	mErrRet( "Could not open output file" )
    }

    *sdo.ostrm << std::fixed;
    BufferString buf;
    for ( int locidx=0; locidx<ps.size(); locidx++ )
    {
	ps[locidx].toString( buf, true );
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
    if ( !ret ) return false;

    uiMSG().message( BufferString("Pickset successfully ",
				  import_ ? "imported" : "exported") );
    return false;
}


bool uiImpExpPickSet::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( import_ && !File::exists(filenm) )
	mErrRet( "Please select input file" );

    if ( !import_ && filenm.isEmpty() )
	mErrRet( "Please select output file" );

    if ( !objfld_->commitInput() )
	return false;

    if ( import_ )
    {
	if ( !dataselfld_->commit() )
	    mErrRet( "Please specify data format" );

	const int zchoice = zfld_->box()->currentItem();
	if ( zchoice == 1 )
	{
	    float constz = constzfld_->getfValue();
	    if ( SI().zIsTime() ) constz /= 1000;

	    if ( !SI().zRange(false).includes( constz,false ) )
		mErrRet( "Please Enter a valid Z value" );
	}
    }

    return true;
}
