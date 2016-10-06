/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/


#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "geom2dascio.h"
#include "dbman.h"
#include "od_istream.h"
#include "posinfo2dsurv.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"

static IOObjContext mkCtxt()
{
    IOObjContext ret( mIOObjContext(SurvGeom2D) );
    return ret;
}

ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage( tr("2D Geometry")),
			       mNoDlgTitle, mODHelpKey(m2DGeomManageDlgHelpID))
			       .nrstatusflds(1).modal(false),mkCtxt())
{
    createDefaultUI( false, false );
    selgrp_->getManipGroup()->addButton( "delete", uiStrings::phrDelete(
		     uiStrings::phrJoinStrings(tr("this"),uiStrings::sLine())),
		     mCB(this,ui2DGeomManageDlg,lineRemoveCB) );
    selgrp_->getManipGroup()->addButton( "browse2dgeom",
	     mJoinUiStrs(sManage(), phrJoinStrings(uiStrings::sLine(),
	     uiStrings::sGeometry())),
	     mCB(this,ui2DGeomManageDlg,manLineGeom) );
}

ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


//-----------Manage Line Geometry-----------------

class uiManageLineGeomDlg : public uiDialog
{ mODTextTranslationClass(uiManageLineGeomDlg);
public:

uiManageLineGeomDlg( uiParent* p, const char* linenm, bool readonly )
    : uiDialog(p,uiDialog::Setup( mJoinUiStrs(sManage(),
				  phrJoinStrings(uiStrings::sLine(),
			          uiStrings::sGeometry())),mNoDlgTitle,
				  mODHelpKey(mManageLineGeomDlgHelpID)))
    , linenm_(linenm),readonly_(readonly)
{
    if ( readonly )
    {
	setCtrlStyle( CloseOnly );
	setCaption( uiStrings::phrJoinStrings(tr("Browse"),
		    mJoinUiStrs(sLine(), sGeometry())) );
    }

    uiString lbl( tr("%1 : %2").arg(mJoinUiStrs(sLine(),sName()))
			       .arg(toUiString(linenm)) );

    uiLabel* titllbl = new uiLabel( this, lbl );
    titllbl->attach( hCentered );

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(linenm) )
    if ( !geom2d )
    {
	uiMSG().error(tr("Cannot find geometry for %1").arg(linenm));
	return;
    }

    const TypeSet<PosInfo::Line2DPos>& positions = geom2d->data().positions();
    table_ = new uiTable( this, uiTable::Setup(positions.size(),3), "2DGeom" );
    table_->attach( ensureBelow, titllbl );
    table_->setPrefWidth( 400 );
    BufferStringSet collbls;
    collbls.add( "Trace Number" ); collbls.add( "X" ); collbls.add( "Y" );
    table_->setColumnLabels( collbls );
    if ( readonly )
	table_->setTableReadOnly( true );

    FloatInpIntervalSpec spec( true );
    rgfld_ = new uiGenInput( this, uiStrings::sZRange(), spec );
    rgfld_->attach( leftAlignedBelow, table_ );
    rgfld_->setValue( geom2d->data().zRange() );
    rgfld_->setReadOnly( readonly );

    if ( !readonly )
    {
	readnewbut_ = new uiPushButton( this, mJoinUiStrs(sImport(),
			phrJoinStrings(uiStrings::sNew(),
			uiStrings::sGeometry())),
			mCB(this,uiManageLineGeomDlg,impLineGeom), true );
	readnewbut_->attach( centeredBelow, rgfld_ );
    }

    fillTable( geom2d->data() );
}


//---------- Import New Geomtery ----------------

class uiGeom2DImpDlg : public uiDialog
{ mODTextTranslationClass(uiGeom2DImpDlg);
public:

uiGeom2DImpDlg( uiParent* p, const char* linenm )
    : uiDialog(p,uiDialog::Setup(mJoinUiStrs(sImport(),
				 phrJoinStrings(uiStrings::sNew(),
				 uiStrings::phrJoinStrings(uiStrings::sLine(),
				 uiStrings::sGeometry()))),
				 toUiString(linenm),
				 mODHelpKey(mGeom2DImpDlgHelpID)))
{
    setOkText( uiStrings::sImport() );
    Table::FormatDesc* geomfd = Geom2dAscIO::getDesc();
    geom2dinfld_ = new uiFileInput( this, mJoinUiStrs(s2D(), phrJoinStrings(
				   uiStrings::sGeometry(), uiStrings::sFile())),
				   uiFileInput::Setup().withexamine(true) );
    dataselfld_ = new uiTableImpDataSel( this, *geomfd, mNoHelpKey );
    dataselfld_->attach( alignedBelow, geom2dinfld_ );
}

bool acceptOK()
{
    if ( File::isEmpty(geom2dinfld_->fileName()) )
    { uiMSG().error(uiStrings::sInvInpFile()); return false; }
    return true;
}

    uiFileInput*	geom2dinfld_;
    uiTableImpDataSel*	dataselfld_;
};


void impLineGeom( CallBacker* )
{
    if ( readonly_ )
	return;

    uiGeom2DImpDlg dlg( this, linenm_ );
    if ( !dlg.go() ) return;

    BufferString filenm( dlg.geom2dinfld_->fileName() );
    if ( !filenm.isEmpty() )
    {
	od_istream strm( filenm );
	if ( !strm.isOK() )
	{ uiMSG().error(uiStrings::sCantOpenInpFile()); return; }

	PosInfo::Line2DData geom( linenm_ );
	Geom2dAscIO geomascio( dlg.dataselfld_->desc(), strm );
	if ( !geomascio.getData( geom ) )
	    uiMSG().error(uiStrings::phrCannotRead( toUiString(filenm)) );

	table_->clearTable();
	fillTable( geom );
    }
}


void fillTable( const PosInfo::Line2DData& geom )
{
    const TypeSet<PosInfo::Line2DPos>& positions = geom.positions();
    table_->setNrRows( positions.size() );
    for ( int idx=0; idx<positions.size(); idx++ )
    {
	table_->setValue( RowCol(idx,0), positions[idx].nr_ );
	table_->setValue( RowCol(idx,1), positions[idx].coord_.x_ );
	table_->setValue( RowCol(idx,2), positions[idx].coord_.y_ );
    }
}


bool acceptOK()
{
    if (!uiMSG().askGoOn(tr("Do you really want to change the geometry?\n"
			    "This will affect all associated data.")))
	return false;

    Pos::GeomID geomid = Survey::GM().getGeomID( linenm_ );
    mDynamicCastGet(Survey::Geometry2D*,geom2d,
		    Survey::GMAdmin().getGeometry(geomid) )
    if ( !geom2d )
	return true;

    PosInfo::Line2DData& geomdata = geom2d->dataAdmin();
    geomdata.setEmpty();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	PosInfo::Line2DPos l2d( table_->getIntValue(RowCol(idx,0)) );
	l2d.coord_.x_ = table_->getDValue( RowCol(idx,1) );
	l2d.coord_.y_ = table_->getDValue( RowCol(idx,2) );
	geomdata.add( l2d );
    }

    geomdata.setZRange( rgfld_->getFStepInterval() );
    geom2d->touch();

    uiString errmsg;
    if ( !Survey::GMAdmin().write(*geom2d,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }
    return true;
}

    const char*		linenm_;
    bool		readonly_;
    uiTable*		table_;
    uiGenInput*		rgfld_;
    uiPushButton*	readnewbut_;
};


//-----------------------------------------------------


void ui2DGeomManageDlg::manLineGeom( CallBacker* )
{
    if ( !curioobj_ ) return;

    PtrMan<Translator> transl = curioobj_->createTranslator();
    if ( !transl )
	return;

    uiManageLineGeomDlg dlg( this, curioobj_->name(),
			     !transl->isUserSelectable(false) );
    dlg.go();
}


void ui2DGeomManageDlg::ownSelChg()
{
}


void ui2DGeomManageDlg::mkFileInfo()
{
    const BufferString txt = getFileInfo();
    setInfo( txt );
}


void ui2DGeomManageDlg::lineRemoveCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const bool docont = uiMSG().askContinue(
       tr("All selected 2D geometries will be deleted.\n"
	  "This will invalidate all other data associated with this geometry"));
    if ( !docont ) return;

    MouseCursorChanger chgr( MouseCursor::Wait );
    uiStringSet msgs;
    DBKeySet selids;
    selgrp_->getChosen( selids );
    for ( int idx=0; idx<selids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = DBM().get( selids[idx] );
	if ( !ioobj || ioobj->implReadOnly() )
	    continue;

	const BufferString lnm( ioobj->name() );
	Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	    return;

	if ( !fullImplRemove(*ioobj) )
	{
	    msgs += tr("Cannot remove %1").arg(lnm);
	    continue;
	}

	DBM().removeEntry( ioobj->key() );
	Survey::GMAdmin().removeGeometry( geomid );
	const FixedString crfromstr = ioobj->pars().find( sKey::CrFrom() );
	if ( !crfromstr.isEmpty() )
	{
	    PosInfo::Line2DKey l2dkey;
	    l2dkey.fromString( crfromstr );
	    if ( l2dkey.isOK() )
	    {
		PosInfo::Survey2D& old2dadmin = PosInfo::POS2DAdmin();
		if ( old2dadmin.curLineSetID() != l2dkey.lsID() )
		    old2dadmin.setCurLineSet( l2dkey.lsID() );

		old2dadmin.removeLine( l2dkey.lineID() );
	    }
	}
    }

    chgr.restore();
    selgrp_->fullUpdate( DBKey::getInvalid() );

    if ( !msgs.isEmpty() )
	uiMSG().errorWithDetails(msgs);
}


//Geom2DImpHandler

Pos::GeomID Geom2DImpHandler::getGeomID( const char* nm, bool ovwok )
{
    Pos::GeomID geomid = Survey::GM().getGeomID( nm );
    if (  geomid == mUdfGeomID )
	return createNewGeom( nm );

    if ( ovwok || confirmOverwrite(nm) )
	setGeomEmpty( geomid );

    return geomid;
}


bool Geom2DImpHandler::getGeomIDs( const BufferStringSet& nms,
				     TypeSet<Pos::GeomID>& geomids, bool ovwok )
{
    geomids.erase();
    BufferString existingidxs;
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	Pos::GeomID geomid = Survey::GM().getGeomID( nms.get(idx) );
	if ( geomid != mUdfGeomID )
	    existingidxs += idx;
	else
	{
	    geomid = createNewGeom( nms.get(idx) );
	    if ( geomid == mUdfGeomID )
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
    mDynamicCastGet( Survey::Geometry2D*, geom2d,
		     Survey::GMAdmin().getGeometry(geomid) );
    if ( !geom2d )
	return;

    geom2d->dataAdmin().setEmpty();
    geom2d->touch();
}


Pos::GeomID Geom2DImpHandler::createNewGeom( const char* nm )
{
    PosInfo::Line2DData* l2d = new PosInfo::Line2DData( nm );
    Survey::Geometry2D* newgeom = new Survey::Geometry2D( l2d );
    uiString msg;
    Pos::GeomID geomid = Survey::GMAdmin().addNewEntry( newgeom, msg );
    if ( geomid == mUdfGeomID )
	uiMSG().error( msg );

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

    return uiMSG().askOverwrite( msg );
}


bool Geom2DImpHandler::confirmOverwrite( const char* lnm )
{
    uiString msg = tr("The 2D Line '%1' already exists. If you overwrite "
		      "its geometry, all the associated data will be "
		      "affected. Do you still want to overwrite?")
		      .arg(lnm);
    return uiMSG().askOverwrite( msg );
}
