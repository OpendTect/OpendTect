/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "geom2dascio.h"
#include "ioman.h"
#include "od_istream.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"

static IOObjContext mkCtxt()
{
    IOObjContext ret( mIOObjContext(Survey::SurvGeom) );
    ret.toselect.allowtransls_= Survey::dgb2DSurvGeomTranslator::translKey();
    return ret;
}

ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage 2D Geometry",mNoDlgTitle,
				 "103.1.14"),mkCtxt())
{
    createDefaultUI( false, false );
    selgrp_->getManipGroup()->addButton( "trashcan", "Remove this Line",
			 mCB(this,ui2DGeomManageDlg,lineRemoveCB) );
    selgrp_->getManipGroup()->addButton( "browse2dgeom", "Manage Line Geometry",
			 mCB(this,ui2DGeomManageDlg,manLineGeom) );
}

ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


//-----------Manage Line Geometry-----------------

class uiManageLineGeomDlg : public uiDialog
{
public:

uiManageLineGeomDlg( uiParent* p, const char* linenm )
    : uiDialog( p, uiDialog::Setup("Manage Line Geometry",
				   mNoDlgTitle,"103.1.15"))
    , linenm_(linenm)
{
    BufferString lbl( "Linename : ");
    lbl.add( linenm );

    uiLabel* titllbl = new uiLabel( this, lbl );
    titllbl->attach( hCentered );

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(linenm) )
    if ( !geom2d )
    {
	uiMSG().error( "Cannot find geometry for ", linenm );
	return;
    }

    const TypeSet<PosInfo::Line2DPos>& positions = geom2d->data().positions();
    table_ = new uiTable( this, uiTable::Setup(positions.size(),3), "2DGeom" );
    table_->attach( ensureBelow, titllbl );
    table_->setPrefWidth( 400 );
    BufferStringSet collbls;
    collbls.add( "Trace Number" ); collbls.add( "X" ); collbls.add( "Y" );
    table_->setColumnLabels( collbls );

    FloatInpIntervalSpec spec( true );
    rgfld_ = new uiGenInput( this, "Z-Range", spec );
    rgfld_->attach( leftAlignedBelow, table_ );
    rgfld_->setValue( geom2d->data().zRange() );

    readnewbut_ = new uiPushButton( this, "Read New Geometry ...",
			mCB(this,uiManageLineGeomDlg,impLineGeom), true );
    readnewbut_->attach( centeredBelow, rgfld_ );

    fillTable( geom2d->data() );
}


//---------- Import New Geomtery ----------------

class uiGeom2DImpDlg : public uiDialog
{
public:

uiGeom2DImpDlg( uiParent* p, const char* linenm )
    : uiDialog(p,uiDialog::Setup("Read new Line Geometry",linenm,"103.1.16"))
{
    Table::FormatDesc* geomfd = Geom2dAscIO::getDesc();
    geom2dinfld_ = new uiFileInput( this, "2D geometry File",
				    uiFileInput::Setup().withexamine(true) );
    dataselfld_ = new uiTableImpDataSel( this, *geomfd, "" );
    dataselfld_->attach( alignedBelow, geom2dinfld_ );
}

bool acceptOK( CallBacker* )
{
    if ( File::isEmpty(geom2dinfld_->fileName()) )
	{ uiMSG().error( "Invalid input file" ); return false; }
    return true;
}

    uiFileInput*	geom2dinfld_;
    uiTableImpDataSel*	dataselfld_;
};


void impLineGeom( CallBacker* )
{
    uiGeom2DImpDlg dlg( this, linenm_ );
    if ( !dlg.go() ) return;

    BufferString filenm( dlg.geom2dinfld_->fileName() );
    if ( !filenm.isEmpty() )
    {
	od_istream strm( filenm );
	if ( !strm.isOK() )
	    { uiMSG().error( "Cannot open input file" ); return; }

	PosInfo::Line2DData geom( linenm_ );
	Geom2dAscIO geomascio( dlg.dataselfld_->desc(), strm );
	if ( !geomascio.getData( geom ) )
	    uiMSG().error( "Failed to convert into compatible data" );

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
	table_->setValue( RowCol(idx,1), positions[idx].coord_.x );
	table_->setValue( RowCol(idx,2), positions[idx].coord_.y );
    }
}


bool acceptOK( CallBacker* )
{
    if ( !uiMSG().askGoOn("Do you really want to change the geometry?\n"
			  "This will affect all associated data.") )
	return false;

    Pos::GeomID geomid = Survey::GM().getGeomID( linenm_ );
    mDynamicCastGet(Survey::Geometry2D*,geom2d,
		    Survey::GMAdmin().getGeometry(geomid) )
    if ( !geom2d )
	return true;

    PosInfo::Line2DData& geomdata = geom2d->data();
    geomdata.setEmpty();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	PosInfo::Line2DPos l2d( table_->getIntValue(RowCol(idx,0)) );
	l2d.coord_.x = table_->getdValue( RowCol(idx,1) );
	l2d.coord_.y = table_->getdValue( RowCol(idx,2) );
	geomdata.add( l2d );
    }

    geomdata.setZRange( rgfld_->getFStepInterval() );
    BufferString errmsg;
    if ( !Survey::GMAdmin().write(*geom2d,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }
    return true;
}

    const char*		linenm_;
    uiTable*		table_;
    uiGenInput*		rgfld_;
    uiPushButton*	readnewbut_;
};


//-----------------------------------------------------


void ui2DGeomManageDlg::manLineGeom( CallBacker* )
{
    if ( !curioobj_ ) return;

    uiManageLineGeomDlg dlg( this, curioobj_->name() );
    dlg.go();
}


void ui2DGeomManageDlg::ownSelChg()
{
}


void ui2DGeomManageDlg::mkFileInfo()
{
}


void ui2DGeomManageDlg::lineRemoveCB( CallBacker* cb )
{
    if ( !curioobj_ ) return;
    const BufferString lnm( curioobj_->name() );
    Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
    if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	return;

    if ( !uiMSG().askGoOn("Do you really want to remove this line?\n"
	    "This will invalidate all other data associated with this line.") )
	return;

    if ( !fullImplRemove(*curioobj_) )
    {
	uiMSG().error( "Cannot remove this line from the database" );
	return;
    }

    IOM().permRemove( curioobj_->key() );
    Survey::GMAdmin().removeGeometry( geomid );
    selgrp_->fullUpdate( curioobj_->key() );
}
