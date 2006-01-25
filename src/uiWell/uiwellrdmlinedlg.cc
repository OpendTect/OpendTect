/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          October 2005
 RCS:           $Id: uiwellrdmlinedlg.cc,v 1.5 2006-01-25 07:13:40 cvskris Exp $
________________________________________________________________________

-*/

#include "uiwellrdmlinedlg.h"

#include "uilistbox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uitable.h"
#include "userinputobj.h"
#include "uimsg.h"
#include "pixmap.h"
#include "oddirs.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
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
    : uiDialog(p,uiDialog::Setup("Random line - Wells relations","",
				 "109.0.0").modal(false))
    , wellsbox_(0), selwellsbox_(0), wellserv_(ws)
{
    BufferString title( "Select wells to set up the random line path" );
    setTitleText( title );

    uiGroup* topgrp = new uiGroup( this, "selection group" );
    uiGroup* selbuttons = new uiGroup( topgrp, "select buttons" );
    uiGroup* movebuttons = new uiGroup( topgrp, "move buttons" );
    createSelectButtons( selbuttons );
    createMoveButtons( movebuttons );
    createFields( topgrp );
    attachFields( selbuttons, topgrp, movebuttons );

    fillListBox();
}


void uiWell2RandomLineDlg::createFields( uiGroup* topgrp )
{
    wellsbox_ = new uiListBox( topgrp, "Available Wells", true );
    selwellsbox_ = 
	new uiTable( topgrp, uiTable::Setup().selmode( 
		    				uiTable::SelectionMode(3) ) );
    selwellsbox_->setNrCols( 2 );
    selwellsbox_->setColumnLabel( 0, "Well Name" );
    selwellsbox_->setColumnLabel( 1, "Read Order" );
    selwellsbox_->setNrRows( 5 );
    
    onlytopfld_ = new uiGenInput( this, "use only wells' top position", 
				  BoolInpSpec( "Yes","No" ) );

    CallBack cb = mCB(this,uiWell2RandomLineDlg,previewPush);
    previewbutton_ = new uiPushButton( this, "Preview", cb );
}


void uiWell2RandomLineDlg::createSelectButtons( uiGroup* selbuttons )
{
    const ioPixmap pm0( GetIconFileName("rightarrow.png") );
    const ioPixmap pm1( GetIconFileName("leftarrow.png") );

    uiLabel* sellbl = new uiLabel( selbuttons, "Select" );
    CallBack cb = mCB(this,uiWell2RandomLineDlg,selButPush);
    toselect_ = new uiToolButton( selbuttons, "", pm0, cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->setHSzPol( uiObject::undef );
    fromselect_ = new uiToolButton( selbuttons, "", pm1, cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::undef );
    selbuttons->setHAlignObj( toselect_ );
}


void uiWell2RandomLineDlg::createMoveButtons( uiGroup* movebuttons )
{
    const ioPixmap pm0( GetIconFileName("upwardarrow.png") );
    const ioPixmap pm1( GetIconFileName("downwardarrow.png") );

    uiLabel* movelbl = new uiLabel( movebuttons, "Change \n order" );
    CallBack cb = mCB(this,uiWell2RandomLineDlg,moveButPush);
    moveupward_ = new uiToolButton( movebuttons, "", pm0, cb );
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::undef );
    movedownward_ = new uiToolButton( movebuttons, "", pm1, cb );
    movedownward_->attach( alignedBelow, moveupward_ );
    movedownward_->setHSzPol( uiObject::undef );
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
}


#define mRC(row,col) RowCol(row,col)

#define mInsertRow(rowidx,text,val)\
	selwellsbox_->insertRows( rowidx );\
	selwellsbox_->setText( mRC(rowidx, 0), text );\
	UserInputObj* uiobj = selwellsbox_->mkUsrInputObj( mRC(rowidx,1) );\
	uiobj->addItems( sTypes );\
	uiobj->setValue( val );
	



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
		selwellsbox_->insertRows( selwellsbox_->nrRows() );
	    }
	    selwellsbox_->setText(mRC(emptyrow, 0), wellsbox_->textOfItem(idx));
	    UserInputObj* uiobj = selwellsbox_->mkUsrInputObj( mRC(emptyrow,1));
	    uiobj->addItems( sTypes );
	    uiobj->setValue( 0 );
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
	    if ( !strcmp( selwellsbox_->text( mRC(idx,0) ), "" ) ) continue;
	    wellsbox_->addItem( selwellsbox_->text( mRC(idx,0) ) );
	    selwellsbox_->removeRow(idx);
	    lastusedidx = idx;
	    wellsbox_->setSelected( wellsbox_->size()-1 );
	    wellsbox_->sort();
	    break;
	}
	while ( selwellsbox_->nrRows() < 5 )
	    selwellsbox_->insertRows( selwellsbox_->nrRows() );

	int selectidx;
	
	if ( lastusedidx<1 )
	    selectidx = 0;
	else if ( lastusedidx<selwellsbox_->nrRows() )
	    selectidx = lastusedidx-1;
	else
	    selectidx = selwellsbox_->nrRows()-1;
	
	selwellsbox_->selectRow( selectidx );
    }
}


void uiWell2RandomLineDlg::fillListBox()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt.forread = true;
    IOM().to( MultiID(IOObjContext::getStdDirData(ctio->ctxt.stdseltype)->id) );
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
	const char* txt = selwellsbox_->text( mRC(idx,0) );
	int wellidx = allwellsnames_.indexOf( txt );
	if ( wellidx<0 ) continue;
	if ( selwellsids_.addIfNew( allwellsids_[wellidx] ) )
	    selwellstypes_ += selwellsbox_->getIntValue( mRC(idx,1) );
    }
}


void uiWell2RandomLineDlg::getCoordinates( TypeSet<Coord>& coords )
{
    setSelectedWells();
    for ( int idx=0; idx<selwellsids_.size(); idx++ )
    {
	Well::Data* wd;
	wd = Well::MGR().get( selwellsids_[idx] );
	if ( onlytopfld_->getBoolValue() )
	{
	    Coord3 coord3 = wd->track().pos(0);
	    coords += Coord( coord3.x, coord3.y );
	}
	else
	{
	    if ( selwellstypes_[idx] )
	    {
		for ( int idx=wd->track().size()-1; idx>=0; idx-- )
		{
		    Coord3 coord3 = wd->track().pos(idx);
		    coords += Coord( coord3.x, coord3.y );
		}
	    }
	    else
	    {
		for ( int idx=0; idx<wd->track().size(); idx++ )
		{
		    Coord3 coord3 = wd->track().pos(idx);
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


void uiWell2RandomLineDlg::moveButPush( CallBacker* cb )
{
    int index = selwellsbox_->currentRow();
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( !selwellsbox_->isRowSelected( index ) ) return;
    
    if ( !strcmp( selwellsbox_->text( mRC(index,0) ), "" ) ) return;
    
    const UserInputObj* obj = selwellsbox_->usrInputObj( mRC(index,1) );
    int value = obj ? obj->getIntValue(): 0;
    const char* text = selwellsbox_->text( mRC(index,0) );

    if ( but == moveupward_ && index>0 )
    {
	mInsertRow( index-1, text, value );
	selwellsbox_->removeRow( index+1 );
	selwellsbox_->selectRow( index-1 );
	selwellsbox_->setCurrentCell( mRC(index-1, 0) );
    }
    else if ( but == movedownward_ && index<selwellsbox_->nrRows() )
    {
	mInsertRow( index+2, text, value );
	selwellsbox_->removeRow( index );
	selwellsbox_->selectRow( index+1 );
	selwellsbox_->setCurrentCell( mRC(index+1, 0) );
    }
}


int uiWell2RandomLineDlg::getFirstEmptyRow()
{
    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	if ( strcmp( selwellsbox_->text( mRC(idx, 0) ), "" ) ) continue;
	return idx;
    }
    
    return -1;
}
