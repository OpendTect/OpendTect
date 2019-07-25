/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2014
________________________________________________________________________

-*/


#include "uiimpexp2dgeom.h"

#include "uifilesel.h"
#include "uigeninput.h"
#include "uigeom2dsel.h"
#include "uiioobjselgrp.h"
#include "uimsg.h"
#include "uitblimpexpdatasel.h"
#include "uilistbox.h"

#include "file.h"
#include "od_iostream.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "survgeom2dascio.h"
#include "survgeometrytransl.h"
#include "tabledef.h"


// uiImp2DGeom
uiImp2DGeom::uiImp2DGeom( uiParent* p, const char* lnm )
    : uiDialog(p,uiDialog::Setup(tr("Import New Line Geometry"),
				 mNoDlgTitle,
				 mODHelpKey(mGeom2DImpDlgHelpID)))
    , singlemultifld_(0)
    , linefld_(0)
    , linenm_(lnm)
    , geomfd_(0)
{
    const FixedString linenm( lnm );
    const bool lineknown = !linenm.isEmpty();
    if ( lineknown )
    {
	uiString title = tr("Set new geometry for %1").arg( linenm );
	setTitleText( title );
    }

    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    geomfd_ = Geom2DAscIO::getDesc( linenm.isEmpty() );
    fnmfld_ = new uiFileSel( this, tr("Input Geometry File"),
				   uiFileSel::Setup().withexamine(true) );
    uiObject* attachobj = fnmfld_->attachObj();
    if ( !lineknown )
    {
	singlemultifld_ =
	    new uiGenInput( this, tr("File contains geometry for"),
		    BoolInpSpec(false,tr("Single line"),tr("Multiple lines")) );
	singlemultifld_->attach( alignedBelow, fnmfld_ );
	mAttachCB( singlemultifld_->valuechanged, uiImp2DGeom::singmultCB );
	mAttachCB( postFinalise(), uiImp2DGeom::singmultCB );
	attachobj = singlemultifld_->attachObj();

	linefld_ = new uiGeom2DSel( this, false );
    }

    dataselfld_ = new uiTableImpDataSel( this, *geomfd_, mNoHelpKey );
    dataselfld_->attach( alignedBelow, attachobj );

    if ( linefld_ )
	linefld_->attach( alignedBelow, dataselfld_ );
}


uiImp2DGeom::~uiImp2DGeom()
{
    detachAllNotifiers();
    delete geomfd_;
}


void uiImp2DGeom::singmultCB( CallBacker* )
{
    const bool singleline = singlemultifld_->getBoolValue();
    linefld_->display( singleline );
    Geom2DAscIO::fillDesc( *geomfd_, !singleline );
}


bool uiImp2DGeom::acceptOK()
{
    if ( File::isEmpty(fnmfld_->fileName()) )
	{ uiMSG().error(uiStrings::sInvInpFile()); return false; }

    if ( !linenm_.isEmpty() )
	return false;

    if ( singlemultifld_->getBoolValue() )
    {
	const IOObj* ioobj = linefld_->ioobj();
	if ( !ioobj )
	    return false;

	const BufferString linenm = ioobj->name();
	const Pos::GeomID geomid = Geom2DImpHandler::getGeomID( linenm );
	if ( geomid == mUdfGeomID )
	    return false;

	const auto& geom2d = SurvGeom::get2D( geomid );
	if ( !fillGeom(const_cast<SurvGeom2D&>(geom2d)) )
	    return false;

	uiString errmsg;
	if ( !Survey::GMAdmin().save(geom2d,errmsg) )
	    { uiMSG().error( errmsg ); return false; }

	uiMSG().message( tr("Line %1 successfully imported.").arg(linenm) );
    }
    else
    {
	ObjectSet<SurvGeom2D> geoms;
	if ( !fillGeom(geoms) )
	    return false;

	BufferStringSet linenms;
	for ( int idx=0; idx<geoms.size(); idx++ )
	    linenms.add( geoms[idx]->name() );

	GeomIDSet geomids;
	if ( !Geom2DImpHandler::getGeomIDs(linenms,geomids) )
	    return false;

	uiStringSet errors;
	for ( int idx=0; idx<geoms.size(); idx++ )
	{
	    uiString errmsg;
	    if ( !Survey::GMAdmin().save(*geoms[idx],errmsg) )
		errors.add( errmsg );
	}

	if ( !errors.isEmpty() )
	{
	    uiMSG().errorWithDetails( errors, tr("Error during import:") );
	    return false;
	}

	uiMSG().message( tr("Lines successfully imported.") );
    }

    return false;
}


od_istream* uiImp2DGeom::getStrm() const
{
     BufferString filenm( fnmfld_->fileName() );
    if ( filenm.isEmpty() )
	return 0;

    od_istream* strm = new od_istream( filenm );
    if ( !strm->isOK() )
    {
	uiMSG().error( uiStrings::phrCannotOpenInpFile() );
	delete strm;
	return 0;
    }

    return strm;
}


bool uiImp2DGeom::fillGeom( ObjectSet<SurvGeom2D>& geoms )
{
    PtrMan<od_istream> strm = getStrm();
    Geom2DAscIO geomascio( dataselfld_->desc(), *strm );
    if ( !geomascio.getData(geoms) )
    {
	uiMSG().error( geomascio.errMsg() );
	return false;
    }

    return true;
}


bool uiImp2DGeom::fillGeom( SurvGeom2D& geom )
{
    PtrMan<od_istream> strm = getStrm();
    Geom2DAscIO geomascio( dataselfld_->desc(), *strm );
    if ( !geomascio.getData(geom) )
    {
	uiMSG().error( geomascio.errMsg() );
	return false;
    }

    return true;
}


// uiExp2DGeom
uiExp2DGeom::uiExp2DGeom( uiParent* p, const TypeSet<Pos::GeomID>* g,
				       bool ismodal )
    : uiDialog(p,Setup(uiStrings::phrExport( tr("2D Geometry")),
		       mNoDlgTitle, mODHelpKey(mExp2DGeomHelpID))
		 .modal(ismodal))
{
    createUI();
    if ( g )
    {
	geomidset_=*g;
	mAttachCB( postFinalise(),uiExp2DGeom::setList );
    }
}


uiExp2DGeom::~uiExp2DGeom()
{
}


void uiExp2DGeom::setList( CallBacker* )
{
    geomfld_->getListField()->setEmpty();
    BufferStringSet linenms;
    for ( int idx=0; idx<geomidset_.size(); idx++ )
    {
	linenms.add( geomidset_.get(idx).name() );
    }
    geomfld_->getListField()->addItems( linenms );
}

void uiExp2DGeom::createUI()
{
    IOObjContext ctxt = mIOObjContext( SurvGeom2D  );
    geomfld_ = new uiIOObjSelGrp( this, ctxt,
				  uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    uiFileSel::Setup fss; fss.setForWrite();
    outfld_ = new uiFileSel( this, uiStrings::sOutputFile(), fss );
    outfld_->attach( alignedBelow, geomfld_ );
}


bool uiExp2DGeom::acceptOK()
{
    const BufferString fnm = outfld_->fileName();
    if ( fnm.isEmpty() )
    {
	uiMSG().error( tr("Please enter the output file name") );
	return false;
    }

    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr("Cannot open the output file") );
	return false;
    }

    BufferString outstr;
    DBKeySet dbkys; geomfld_->getChosen( dbkys );
    for ( int gidx=0; gidx<dbkys.size(); gidx++ )
    {
	const auto geomid = geomIDOf( dbkys[gidx] );
	const auto& geom2d = SurvGeom::get2D( geomid );
	if ( geom2d.isEmpty() )
	    continue;

	const PosInfo::Line2DData& data2d = geom2d.data();
	const TypeSet<PosInfo::Line2DPos>& allpos = data2d.positions();

	for ( int pidx=0; pidx<allpos.size(); pidx++ )
	{
	    const PosInfo::Line2DPos& pos = allpos[pidx];
	    outstr.setEmpty();
	    const BufferString xcrdstr = toString( pos.coord_.x_ );
	    const BufferString ycrdstr = toString( pos.coord_.y_ );
	    outstr.add( data2d.lineName() ).addTab()
		  .add( pos.nr_ ).addTab()
		  .add( xcrdstr.buf() ).addTab()
		  .add( ycrdstr.buf() );
	    strm << outstr.buf() << '\n';
	}
    }

    strm.close();

    const uiString msg = tr("Geometry successfully exported.\n\n"
			    "Do you want to export more?");
    const bool res = uiMSG().askGoOn( msg );
    return !res;
}


//Geom2DImpHandler

Pos::GeomID Geom2DImpHandler::getGeomID( const char* nm, bool ovwok )
{
    Pos::GeomID geomid = SurvGeom::getGeomID( nm );
    if ( !geomid.isValid() )
	return createNewGeom( nm );

    if ( ovwok || confirmOverwrite(nm) )
	setGeomEmpty( geomid );

    return geomid;
}


bool Geom2DImpHandler::getGeomIDs( const BufferStringSet& nms,
				   GeomIDSet& geomids, bool ovwok )
{
    geomids.erase();
    TypeSet<int> existingidxs;
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	Pos::GeomID geomid = SurvGeom::getGeomID( nms.get(idx) );
	if ( !mIsUdfGeomID(geomid) )
	    existingidxs += idx;
	else
	{
	    geomid = createNewGeom( nms.get(idx) );
	    if ( mIsUdfGeomID(geomid) )
		return false;
	}

	geomids += geomid;
    }

    if ( !existingidxs.isEmpty() )
    {
	BufferStringSet existinglnms;
	for ( int idx=0; idx<existingidxs.size(); idx++ )
	    existinglnms.add( nms.get(existingidxs[idx]) );

	if ( ovwok || confirmOverwrite(existinglnms) )
	{
	    for ( int idx=0; idx<existingidxs.size(); idx++ )
		setGeomEmpty( geomids[existingidxs[idx]] );
	}
    }

    return true;
}


void Geom2DImpHandler::setGeomEmpty( Pos::GeomID geomid )
{
    const auto& cgeom2d = SurvGeom::get2D( geomid );
    if ( !cgeom2d.isEmpty() )
    {
	auto& geom2d = const_cast<SurvGeom2D&>( cgeom2d );
	geom2d.data().setEmpty();
	geom2d.commitChanges();
    }
}


Pos::GeomID Geom2DImpHandler::createNewGeom( const char* nm )
{
    PosInfo::Line2DData* l2d = new PosInfo::Line2DData( nm );
    SurvGeom2D* newgeom = new SurvGeom2D( l2d );

    uiString msg; Pos::GeomID geomid;
    if ( Survey::GMAdmin().addEntry(newgeom,geomid,msg) )
	gUiMsg().error( msg );

    return geomid;
}


bool Geom2DImpHandler::confirmOverwrite( const BufferStringSet& lnms )
{
    if ( lnms.size() == 1 )
	return confirmOverwrite( lnms.get(0) );

    uiString msg =
	tr("The 2D Lines %1 already exist. If you overwrite "
	   "their geometry, all the associated data will be "
	   "affected. Do you still want to overwrite?")
	.arg(lnms.getDispString(5));

    return gUiMsg().askOverwrite( msg );
}


bool Geom2DImpHandler::confirmOverwrite( const char* lnm )
{
    uiString msg = tr("The 2D Line '%1' already exists. If you overwrite "
		      "its geometry, all the associated data will be "
		      "affected. Do you still want to overwrite?")
		      .arg(lnm);
    return gUiMsg().askOverwrite( msg );
}
