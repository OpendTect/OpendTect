/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "file.h"
#include "geom2dascio.h"
#include "surv2dgeom.h"
#include "strmprov.h"

#include "uitoolbutton.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"

static const char* remmsg = "All the related 2D lines & horizons will become invalid. Do you want to go ahead?";

ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Manage 2D Geometry",mNoDlgTitle,
				 "103.1.14"))
{
    setCtrlStyle( LeaveOnly );

    BufferStringSet linesets;
    S2DPOS().getLineSets( linesets );
    uiLabeledListBox* lslb =
	new uiLabeledListBox( this, linesets, "Linesets", false,
			      uiLabeledListBox::AboveMid );
    linesetfld_ = lslb->box();
    linesetfld_->selectionChanged.notify(
	    mCB(this,ui2DGeomManageDlg,lineSetSelCB) );
    linesetfld_->setPrefWidth( 200 );
    
	uiToolButton* removelsgeombut =
	new uiToolButton( this, "trashcan", "Reemove LineSet Geometry",
			  mCB(this,ui2DGeomManageDlg,removeLineSetGeom) );
    removelsgeombut->attach( centeredRightOf, lslb );
	
	uiSeparator* versep = new uiSeparator( this, "", false );
    versep->attach( centeredRightOf, removelsgeombut );

    uiLabeledListBox* lnlb =
	new uiLabeledListBox( this, "Linenames", false,
			      uiLabeledListBox::AboveMid );
    lnlb->attach( rightTo, versep );
    linenamefld_ = lnlb->box();
    linenamefld_->setPrefWidth( 200 );
    
    uiToolButton* mangeombut =
	new uiToolButton( this, "browse2dgeom", "Manage Line Geometry",
			  mCB(this,ui2DGeomManageDlg,manLineGeom) );
    mangeombut->attach( centeredRightOf, lnlb );

	uiToolButton* remgeombut =
	new uiToolButton( this, "trashcan", "Remove Line Geometry",
			  mCB(this,ui2DGeomManageDlg,removeLineGeom) );
    remgeombut->attach( alignedBelow, mangeombut );


    lineSetSelCB( 0 );
}


ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


void ui2DGeomManageDlg::lineSetSelCB( CallBacker* )
{
    if ( linesetfld_->isEmpty() )
    {
	linenamefld_->setEmpty();
	return;
    }

    BufferStringSet linenames;
    S2DPOS().setCurLineSet( linesetfld_->getText() );
    S2DPOS().getLines( linenames );
    linenamefld_->setEmpty();
    linenamefld_->addItems( linenames );
}


void ui2DGeomManageDlg::removeLineSetGeom( CallBacker* )
{
    if ( linesetfld_->isEmpty() || !linesetfld_->nrSelected() ||
	 linesetfld_->currentItem() < 0 || !uiMSG().askGoOn(remmsg) )
	    return;

    PosInfo::POS2DAdmin().removeLineSet( linesetfld_->getText() );
    linesetfld_->removeItem( linesetfld_->currentItem() );
    if ( !linesetfld_->isEmpty() )
	linesetfld_->setCurrentItem( 0 );
    lineSetSelCB( 0 );
}

//-----------Manage Line Geometry-----------------


class uiManageLineGeomDlg : public uiDialog
{
    public:
uiManageLineGeomDlg( uiParent* p, const char* linenm )
    : uiDialog( p, uiDialog::Setup("Manage Line Geometry",linenm,"103.1.15"))
    , linenm_(linenm)
{
    BufferString lbl( "Lineset : ");
    lbl.add( S2DPOS().curLineSet() );
    lbl.add( ", Linename : ");
    lbl.add( linenm );
    
    uiLabel* titllbl = new uiLabel( this, lbl );
    titllbl->attach( hCentered );
    
    PosInfo::Line2DData geom( linenm );
    S2DPOS().getGeometry( geom );
    const TypeSet<PosInfo::Line2DPos>& positions = geom.positions();
    table_ = new uiTable( this, uiTable::Setup(positions.size(),3), "2DGeom" );
    table_->attach( ensureBelow, titllbl );
    table_->setPrefWidth( 400 );
    BufferStringSet collbls;
    collbls.add( "Trace Number" ); collbls.add( "X" ); collbls.add( "Y" );
    table_->setColumnLabels( collbls );

    FloatInpIntervalSpec spec( true );
    rgfld_ = new uiGenInput( this, "Z-Range", spec );
    rgfld_->attach( leftAlignedBelow, table_ );
    rgfld_->setValue( geom.zRange() );

    readnewbut_ =
	new uiPushButton( this, "Read New Geometry ...", 
		      	  mCB(this,uiManageLineGeomDlg,impLineGeom), true );
    readnewbut_->attach( centeredBelow, rgfld_ );
    
    fillTable( geom );
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
    {
	uiMSG().error( "Invalid input file" );
	return false;
    }
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
	StreamData sd = StreamProvider( filenm ).makeIStream();
	if ( !sd.usable() )
	{
	    uiMSG().error( "Cannot open input file" );
	    return;
	}

	PosInfo::Line2DData geom( linenm_ );
	Geom2dAscIO geomascio( dlg.dataselfld_->desc(), *sd.istrm );
	if ( !geomascio.getData( geom ) )
	    uiMSG().error( "Failed to convert into compatible data" );

	sd.close();

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
    if ( !uiMSG().askGoOn("Do you really want to change the geometry?") )
	return false;

    PosInfo::Line2DData geom( linenm_ );
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	PosInfo::Line2DPos l2d( table_->getIntValue(RowCol(idx,0)) );
	l2d.coord_.x = table_->getdValue( RowCol(idx,1) );
	l2d.coord_.y = table_->getdValue( RowCol(idx,2) );
	geom.add( l2d );
    }

    geom.setZRange( rgfld_->getFStepInterval() );
    PosInfo::POS2DAdmin().setGeometry( geom );
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
    if ( linenamefld_->isEmpty() || !linenamefld_->nrSelected() ||
	 linenamefld_->currentItem() < 0 )
	return;

    uiManageLineGeomDlg dlg( this, linenamefld_->getText() );
    dlg.go();
}


void ui2DGeomManageDlg::removeLineGeom( CallBacker* )
{
    if ( linenamefld_->isEmpty() || !linenamefld_->nrSelected() ||
	 linenamefld_->currentItem() < 0 || !uiMSG().askGoOn(remmsg) )
	    return;

    PosInfo::POS2DAdmin().removeLine( linenamefld_->getText() );
    linenamefld_->removeItem( linenamefld_->currentItem() );
    linenamefld_->setCurrentItem( 0 );
    lineSetSelCB( 0 );
}
