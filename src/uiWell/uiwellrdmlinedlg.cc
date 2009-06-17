/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          October 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellrdmlinedlg.cc,v 1.23 2009-06-17 06:03:18 cvsranojay Exp $";

#include "uiwellrdmlinedlg.h"

#include "uilistbox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uitable.h"
#include "uimsg.h"
#include "pixmap.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellman.h"
#include "welltrack.h"
#include "welltransl.h"
#include "transl.h"
#include "uiioobjsel.h"
#include "uiwellpartserv.h"


static const char* sTypes[] =
{
    "Normal",
    "Reversed",
    0
};


uiWell2RandomLineDlg::uiWell2RandomLineDlg( uiParent* p, uiWellPartServer* ws )
    : uiDialog(p,uiDialog::Setup("Create Random line",
				 "Select wells to set up the random line path",
				 "109.0.0").modal(false))
    , wellsbox_(0), selwellsbox_(0), wellserv_(ws)
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt.forread = false;

    uiGroup* topgrp = new uiGroup( this, "selection group" );
    uiGroup* selbuttons = new uiGroup( topgrp, "select buttons" );
    uiGroup* movebuttons = new uiGroup( topgrp, "move buttons" );
    createSelectButtons( selbuttons );
    createMoveButtons( movebuttons );
    createFields( topgrp );
    attachFields( selbuttons, topgrp, movebuttons );

    fillListBox();
    ptsSel(0);
}


uiWell2RandomLineDlg::~uiWell2RandomLineDlg()
{
    delete outctio_.ioobj; delete &outctio_;
}


void uiWell2RandomLineDlg::createFields( uiGroup* topgrp )
{
    wellsbox_ = new uiListBox( topgrp, "Available Wells", true );
    selwellsbox_ = new uiTable( topgrp, uiTable::Setup()
				        .selmode(uiTable::SelectionMode(3) ),
				"Wells Table" );
    selwellsbox_->setNrCols( 2 );
    selwellsbox_->setColumnLabel( 0, "Well Name" );
    selwellsbox_->setColumnLabel( 1, "Read Order" );
    selwellsbox_->setNrRows( 5 );
    selwellsbox_->setColumnWidth(0,90);
    selwellsbox_->setColumnWidth(1,90);
    selwellsbox_->setTableReadOnly(true);
    
    onlytopfld_ = new uiGenInput( this, "Use only wells' top position", 
				  BoolInpSpec(true) );
    onlytopfld_->valuechanged.notify( mCB(this,uiWell2RandomLineDlg,ptsSel) );
    onlytopfld_->setValue(false);

    CallBack cb = mCB(this,uiWell2RandomLineDlg,previewPush);
    previewbutton_ = new uiPushButton( this, "&Preview", cb, true );
    outfld_ = new uiIOObjSel( this, outctio_, "Output Random line(s)" );
    dispfld_ = new uiCheckBox( this, "Display Random Line on creation" );
    dispfld_->setChecked( true );
}


void uiWell2RandomLineDlg::createSelectButtons( uiGroup* selbuttons )
{
    const ioPixmap pm0( "rightarrow.png" );
    const ioPixmap pm1( "leftarrow.png" );

    uiLabel* sellbl = new uiLabel( selbuttons, "Select" );
    CallBack cb = mCB(this,uiWell2RandomLineDlg,selButPush);
    toselect_ = new uiToolButton( selbuttons, "Move right", pm0, cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->setHSzPol( uiObject::Undef );
    fromselect_ = new uiToolButton( selbuttons, "Move left", pm1, cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::Undef );
    selbuttons->setHAlignObj( toselect_ );
}


void uiWell2RandomLineDlg::createMoveButtons( uiGroup* movebuttons )
{
    const ioPixmap pm0( "uparrow.png" );
    const ioPixmap pm1( "downarrow.png" );

    uiLabel* movelbl = new uiLabel( movebuttons, "Change \n order" );
    CallBack cb = mCB(this,uiWell2RandomLineDlg,moveButPush);
    moveupward_ = new uiToolButton( movebuttons, "Move Up", pm0, cb );
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::Undef );
    movedownward_ = new uiToolButton( movebuttons, "Move Down", pm1, cb );
    movedownward_->attach( alignedBelow, moveupward_ );
    movedownward_->setHSzPol( uiObject::Undef );
    movebuttons->setHAlignObj( moveupward_ );
}


void uiWell2RandomLineDlg::attachFields( uiGroup* selbuttons, uiGroup* topgrp,
					 uiGroup* movebuttons )
{
    selbuttons->attach( centeredLeftOf, selwellsbox_ );
    selbuttons->attach( ensureRightOf, wellsbox_ );
    selwellsbox_->attach( rightTo, wellsbox_ );
    movebuttons->attach( centeredRightOf, selwellsbox_ );
    onlytopfld_->attach( centeredBelow, topgrp );
    previewbutton_->attach( centeredBelow, onlytopfld_ );
    outfld_->attach( alignedBelow, previewbutton_ );
    dispfld_->attach( alignedBelow, outfld_ );
}


bool uiWell2RandomLineDlg::dispOnCreation()
{
    return dispfld_->isChecked();
}


void uiWell2RandomLineDlg::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( but == toselect_ )
    {
	int lastusedidx = 0;
	for ( int idx=0; idx<wellsbox_->size(); idx++ )
	{
	    if ( !wellsbox_->isSelected(idx) ) continue;
	   
	    int emptyrow = getFirstEmptyRow();
	    if ( emptyrow == -1 )
	    {
		emptyrow = selwellsbox_->nrRows();
		selwellsbox_->insertRows( selwellsbox_->nrRows(), 1 );
	    }
	    selwellsbox_->setText( RowCol(emptyrow,0),
		    		   wellsbox_->textOfItem(idx));
	    uiComboBox* box = new uiComboBox( 0, "Type" );
	    selwellsbox_->setCellObject( RowCol(emptyrow,1), box );
	    box->addItems( sTypes );
	    box->setValue( 0 );
	    wellsbox_->removeItem(idx);
	    lastusedidx = idx;
	    wellsbox_->sort();
	    idx--;
	}
	wellsbox_->setSelected( lastusedidx<wellsbox_->size() ?
			        lastusedidx : wellsbox_->size()-1 );
	int selectidx = getFirstEmptyRow()-1<0 ?
	    		selwellsbox_->nrRows()-1 : getFirstEmptyRow()-1;
	selwellsbox_->selectRow( selectidx );
    }
    else if ( but == fromselect_ )
    {
	int lastusedidx = 0;
	for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
	{
	    if ( !selwellsbox_->isRowSelected(idx) ) continue;
	    if ( !strcmp( selwellsbox_->text( RowCol(idx,0) ), "" ) )
		continue;

	    wellsbox_->addItem( selwellsbox_->text( RowCol(idx,0) ) );
	    selwellsbox_->removeRow(idx);
	    lastusedidx = idx;
	    wellsbox_->selectAll(false);
	    wellsbox_->setSelected( wellsbox_->size()-1 );
	    wellsbox_->sort();
	    break;
	}
	while ( selwellsbox_->nrRows() < 5 )
	    selwellsbox_->insertRows( selwellsbox_->nrRows(), 1 );

	int selectidx;
	
	if ( lastusedidx<1 )
	    selectidx = 0;
	else if ( lastusedidx<selwellsbox_->nrRows() )
	    selectidx = lastusedidx-1;
	else
	    selectidx = selwellsbox_->nrRows()-1;
	
	selwellsbox_->selectRow( selectidx );
    }

    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,box,
			selwellsbox_->getCellObject(RowCol(idx,1)))
	if ( box ) box->setName( BufferString("Type",idx) );
    }
}


void uiWell2RandomLineDlg::fillListBox()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt.forread = true;
    IOM().to( ctio->ctxt.getSelKey() );
    IODirEntryList entrylist( IOM().dirPtr(), ctio->ctxt );

    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	entrylist.setCurrent( idx );
	allwellsids_ += entrylist.selected()->key();
	allwellsnames_.add( entrylist[idx]->name() );
    }

    wellsbox_->addItems( allwellsnames_ );
}


void uiWell2RandomLineDlg::setSelectedWells()
{
    selwellsids_.erase();
    selwellstypes_.erase();

    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	const char* txt = selwellsbox_->text( RowCol(idx,0) );
	int wellidx = allwellsnames_.indexOf( txt );
	if ( wellidx<0 ) continue;
	if ( selwellsids_.addIfNew( allwellsids_[wellidx] ) )
	{
	    mDynamicCastGet(uiComboBox*,box,
			    selwellsbox_->getCellObject(RowCol(idx,1)))
	    selwellstypes_ += box->currentItem();
	}
    }
}


void uiWell2RandomLineDlg::getCoordinates( TypeSet<Coord>& coords )
{
    setSelectedWells();
    for ( int idx=0; idx<selwellsids_.size(); idx++ )
    {
	Well::Data* wd;
	wd = Well::MGR().get( selwellsids_[idx] );
	if ( !wd ) return;
	if ( onlytopfld_->getBoolValue() )
	{
	    Coord3 coord3 = wd->track().pos(0);
	    coords += Coord( coord3.x, coord3.y );
	}
	else
	{
	    if ( selwellstypes_[idx] )
	    {
		for ( int posidx=wd->track().size()-1; posidx>=0; posidx-- )
		{
		    if ( posidx == wd->track().size()-1 )
		    {
			Coord3 coord3 = wd->track().pos( posidx );
			coords += Coord( coord3.x, coord3.y );
			continue;
		    }
		    Coord3 prevcoord3 = wd->track().pos( posidx + 1 );
		    Coord3 coord3 = wd->track().pos( posidx );
		    if ( prevcoord3 != coord3 )
			coords += Coord( coord3.x, coord3.y );
		}
	    }
	    else
	    {
		for ( int posidx=0; posidx<wd->track().size(); posidx++ )
		{
		    if ( posidx == 0 )
		    {
			Coord3 coord3 = wd->track().pos( posidx );
			coords += Coord( coord3.x, coord3.y );
			continue;
		    }
		    Coord3 prevcoord3 = wd->track().pos( posidx - 1 );
		    Coord3 coord3 = wd->track().pos( posidx );
		    if ( prevcoord3 != coord3 )
			coords += Coord( coord3.x, coord3.y );
		}
	    }
	}
    }
}


void uiWell2RandomLineDlg::previewPush( CallBacker* cb )
{
    wellserv_->sendPreviewEvent();
}


#define mInsertRow(rowidx,text,val)\
	selwellsbox_->insertRows( rowidx, 1 );\
	selwellsbox_->setText( RowCol(rowidx,0), text );\
	uiComboBox* newbox = new uiComboBox(0,"Type"); \
	newbox->addItems( sTypes ); \
	newbox->setValue( val ); \
	selwellsbox_->setCellObject( RowCol(rowidx,1), newbox );

void uiWell2RandomLineDlg::moveButPush( CallBacker* cb )
{
    int index = selwellsbox_->currentRow();
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( !selwellsbox_->isRowSelected( index ) ) return;
    
    if ( !strcmp( selwellsbox_->text( RowCol(index,0) ), "" ) ) return;

    mDynamicCastGet(uiComboBox*,box,
		    selwellsbox_->getCellObject(RowCol(index,1)))
    const int value = box ? box->getIntValue(): 0;
    BufferString text = selwellsbox_->text( RowCol(index,0) );

    if ( but == moveupward_ && index>0 )
    {
	mInsertRow( index-1, text.buf(), value );
	selwellsbox_->removeRow( index+1 );
	selwellsbox_->selectRow( index-1 );
	selwellsbox_->setCurrentCell( RowCol(index-1,0) );
    }
    else if ( but == movedownward_ && index<selwellsbox_->nrRows() 
	      && strcmp( selwellsbox_->text( RowCol(index+1,0) ), "" ) )
    {
	mInsertRow( index+2, text.buf(), value );
	selwellsbox_->removeRow( index );
	selwellsbox_->selectRow( index+1 );
	selwellsbox_->setCurrentCell( RowCol(index+1,0) );
    }

    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,box,
			selwellsbox_->getCellObject(RowCol(idx,1)))
	if ( box ) box->setName( BufferString("Type",idx) );
    }
}


int uiWell2RandomLineDlg::getFirstEmptyRow()
{
    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	if ( strcmp( selwellsbox_->text( RowCol(idx,0) ), "" ) ) continue;
	return idx;
    }
    
    return -1;
}


void uiWell2RandomLineDlg::ptsSel( CallBacker* cb )
{
    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,box,
			selwellsbox_->getCellObject(RowCol(idx,1)))
	if ( box ) box->setSensitive( !onlytopfld_->getBoolValue() );
    }
}


bool uiWell2RandomLineDlg::acceptOK( CallBacker* )
{
    if ( !outfld_->commitInput() || !outctio_.ioobj )
    {
	uiMSG().error( " Please specify the output " );
	return false;
    }

    Geometry::RandomLine* rl = new Geometry::RandomLine;
    TypeSet<Coord> wellcoord;
    getCoordinates( wellcoord );
    for ( int idx=0; idx<wellcoord.size(); idx++ )
	rl->addNode( SI().transform(wellcoord[idx]) );

    Geometry::RandomLineSet outrls;
    outrls.addLine( rl );
    BufferString msg;
    const bool  res = RandomLineSetTranslator::store(outrls,outctio_.ioobj,msg);
    if ( !res )
	uiMSG().error(msg);

    return res;
}


const char* uiWell2RandomLineDlg::getRandLineID() const
{
    if ( !outctio_.ioobj )
	return 0;
    BufferString* multid = new BufferString( outctio_.ioobj->key().buf() );
    return multid->buf();
}
