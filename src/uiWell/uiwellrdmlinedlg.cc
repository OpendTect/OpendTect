/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          October 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwellrdmlinedlg.h"

#include "uilistbox.h"
#include "uigeninput.h"
#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uitable.h"
#include "uimsg.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survgeometry.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "welldata.h"
#include "wellman.h"
#include "welltrack.h"
#include "welltransl.h"
#include "transl.h"
#include "uiioobjsel.h"
#include "uiwellpartserv.h"

#include <math.h>


static const char* sTypes[] = { "Top", "Bottom", 0 };

uiWellSelGrp::uiWellSelGrp( uiParent* p, bool withpos )
    : uiGroup(p, "Select wells in table" )
    , withpos_(withpos)  
    , wellsbox_(0), selwellsbox_(0), onlytopfld_(0)			   
{
    uiGroup* selbuttons = new uiGroup( this, "select buttons" );
    uiGroup* movebuttons = new uiGroup( this, "move buttons" );

    createFields();
    createSelectButtons( selbuttons );
    createMoveButtons( movebuttons );

    attachFields( selbuttons, movebuttons );

    fillListBox();
    ptsSel(0);
}


void uiWellSelGrp::createFields()
{
    wellsbox_ = new uiListBox( this, "Available Wells", true );
    selwellsbox_ = new uiTable( this, uiTable::Setup()
				        .selmode(uiTable::SelectionMode(3) ),
				"Wells Table" );
    selwellsbox_->setNrCols( withpos_ ? 2 : 1 );
    selwellsbox_->setNrRows( 5 );
    selwellsbox_->setColumnLabel( 0, "Well Name" );
    selwellsbox_->setColumnWidth(0,90);
    if ( withpos_ )
    {
	selwellsbox_->setColumnLabel( 1, "Start at" );
	selwellsbox_->setColumnWidth(1,90);
	  
	onlytopfld_ = new uiGenInput( this, "Use only wells' top position", 
				      BoolInpSpec(true) );
	onlytopfld_->valuechanged.notify( mCB(this,uiWellSelGrp,ptsSel) );
	onlytopfld_->attach( alignedBelow, selwellsbox_ );
	onlytopfld_->attach( ensureBelow, wellsbox_ );
    }
    selwellsbox_->setTableReadOnly(true);
    setHAlignObj( selwellsbox_ );
}


void uiWellSelGrp::createSelectButtons( uiGroup* selbuttons )
{
    uiLabel* sellbl = new uiLabel( selbuttons, "Select" );
    CallBack cb = mCB(this,uiWellSelGrp,selButPush);
    toselect_ = new uiToolButton( selbuttons, uiToolButton::RightArrow,
					"Move right", cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->setHSzPol( uiObject::Undef );
    fromselect_ = new uiToolButton( selbuttons, uiToolButton::LeftArrow,
	    				"Move left", cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::Undef );
    selbuttons->setHAlignObj( toselect_ );
}


void uiWellSelGrp::createMoveButtons( uiGroup* movebuttons )
{
    uiLabel* movelbl = new uiLabel( movebuttons, "Change \n order" );
    CallBack cb = mCB(this,uiWellSelGrp,moveButPush);
    moveupward_ = new uiToolButton( movebuttons, uiToolButton::UpArrow,
	    			    "Move Up", cb );
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::Undef );
    movedownward_ = new uiToolButton( movebuttons, uiToolButton::DownArrow,
	    				"Move Down", cb );
    movedownward_->attach( alignedBelow, moveupward_ );
    movedownward_->setHSzPol( uiObject::Undef );
    movebuttons->setHAlignObj( moveupward_ );
}


void uiWellSelGrp::attachFields( uiGroup* selbuttons, uiGroup* movebuttons )
{
    selbuttons->attach( centeredLeftOf, selwellsbox_ );
    selbuttons->attach( ensureRightOf, wellsbox_ );
    selwellsbox_->attach( rightTo, wellsbox_ );
    movebuttons->attach( centeredRightOf, selwellsbox_ );
    if ( onlytopfld_ ) onlytopfld_->attach( ensureBelow, selwellsbox_ );
}


void uiWellSelGrp::selButPush( CallBacker* cb )
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
	    idx--;
	}
	wellsbox_->sortItems();
	wellsbox_->setCurrentItem( lastusedidx<wellsbox_->size() ?
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
	    wellsbox_->sortItems();
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


void uiWellSelGrp::fillListBox()
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


void uiWellSelGrp::setSelectedWells()
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
	    if ( box ) selwellstypes_ += box->currentItem();
	}
    }
}


void uiWellSelGrp::getCoordinates( TypeSet<Coord>& coords )
{
    setSelectedWells();
    const bool onlytop = onlytopfld_ ? onlytopfld_->getBoolValue() : false;
    for ( int idx=0; idx<selwellsids_.size(); idx++ )
    {
	const Well::Data* wd = Well::MGR().get( selwellsids_[idx] );
	if ( !wd || wd->track().isEmpty() ) return;

	if ( onlytop )
	    coords += wd->track().pos(0).coord();
	else
	{
	    const int firstidx = selwellstypes_[idx] ? wd->track().size()-1 : 0;
	    const int stopidx = selwellstypes_[idx] ? -1 : wd->track().size();
	    const int incidx = selwellstypes_[idx] ? -1 : 1;

	    BinID prevbid( SI().transform(wd->track().pos(firstidx)) );
	    coords += SI().transform( prevbid );
	    for ( int posidx=firstidx; posidx!=stopidx; posidx+=incidx )
	    {
		BinID bid( SI().transform(wd->track().pos(posidx)) );
		if ( bid != prevbid )
		{
		    coords += SI().transform( bid );
		    prevbid = bid;
		}
	    }
	}
    }
}


#define mInsertRow(rowidx,text,val)\
	selwellsbox_->insertRows( rowidx, 1 );\
	selwellsbox_->setText( RowCol(rowidx,0), text );\
	uiComboBox* newbox = new uiComboBox(0,"Type"); \
	newbox->addItems( sTypes ); \
	newbox->setValue( val ); \
	selwellsbox_->setCellObject( RowCol(rowidx,1), newbox );

void uiWellSelGrp::moveButPush( CallBacker* cb )
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
	mDynamicCastGet(uiComboBox*,wellbox,
			selwellsbox_->getCellObject(RowCol(idx,1)))
	if ( wellbox ) wellbox->setName( BufferString("Type",idx) );
    }
}


int uiWellSelGrp::getFirstEmptyRow()
{
    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	if ( strcmp( selwellsbox_->text( RowCol(idx,0) ), "" ) ) continue;
	return idx;
    }
    
    return -1;
}


void uiWellSelGrp::ptsSel( CallBacker* cb )
{
    for ( int idx=0; idx<selwellsbox_->nrRows(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,box,
			selwellsbox_->getCellObject(RowCol(idx,1)))
	if ( box ) box->setSensitive( !onlytopfld_->getBoolValue() );
    }
}




uiWell2RandomLineDlg::uiWell2RandomLineDlg( uiParent* p, uiWellPartServer* ws )
    : uiDialog(p,uiDialog::Setup("Create Random line",
				 "Select wells to set up the random line path",
				 "109.0.0").modal(false))
    , wellserv_(ws)
    , outctio_(*mMkCtxtIOObj(RandomLineSet))
{
    outctio_.ctxt.forread = false;
    selgrp_ = new uiWellSelGrp( this );

    createFields();
    attachFields();
}


uiWell2RandomLineDlg::~uiWell2RandomLineDlg()
{
    delete outctio_.ioobj; delete &outctio_;
}


void uiWell2RandomLineDlg::createFields()
{
    BufferString txt( "Extend outward (" );
    txt += SI().xyInFeet() ? "ft)" : "m)";
    float defdist = 100 * SI().inlDistance();
    extendfld_ = new uiGenInput(this,txt,FloatInpSpec((float)mNINT32(defdist)));
    extendfld_->setWithCheck( true );
    extendfld_->setChecked( true );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, extendfld_ );

    CallBack cb = mCB(this,uiWell2RandomLineDlg,previewPush);
    previewbutton_ = new uiPushButton( this, "&Preview", cb, true );
    previewbutton_->attach( ensureBelow, sep );
    outfld_ = new uiIOObjSel( this, outctio_, "Output Random line(s)" );
    dispfld_ = new uiCheckBox( this, "Display Random Line on creation" );
    dispfld_->setChecked( true );
}


void uiWell2RandomLineDlg::attachFields()
{
    extendfld_->attach( alignedBelow, selgrp_ );
    previewbutton_->attach( alignedBelow, extendfld_ );
    outfld_->attach( alignedBelow, previewbutton_ );
    dispfld_->attach( alignedBelow, outfld_ );
}


bool uiWell2RandomLineDlg::dispOnCreation()
{
    return dispfld_->isChecked();
}


void uiWell2RandomLineDlg::getCoordinates( TypeSet<Coord>& coords )
{
    selgrp_->getCoordinates( coords );

    if ( extendfld_->isChecked() )
	extendLine( coords );
}


void uiWell2RandomLineDlg::extendLine( TypeSet<Coord>& coords )
{
    const int nrcoords = coords.size();
    if ( nrcoords < 1 ) return;
    float extradist = extendfld_->getfValue();
    if ( extradist < 0.1 || extradist > 1e6 ) return;
    if ( SI().xyInFeet() )
	extradist *= mFromFeetFactorF;
    if ( nrcoords == 1 )
    {
	const Coord c( coords[0] );
	coords.erase();
	coords += Coord( c.x-extradist, c.y );
	coords += c;
	coords += Coord( c.x+extradist, c.y );
    }
    else
    {
	TypeSet<Coord> oldcrds( coords );
	coords.erase();
	const Coord d0( oldcrds[1].x - oldcrds[0].x,
			oldcrds[1].y - oldcrds[0].y );
	float p = (float) sqrt( extradist * extradist / d0.sqAbs() );
	const Coord newc0( oldcrds[0].x - p * d0.x, oldcrds[0].y - p * d0.y );
	const Coord d1( oldcrds[nrcoords-1].x - oldcrds[nrcoords-2].x,
			oldcrds[nrcoords-1].y - oldcrds[nrcoords-2].y );
	p = (float) sqrt( extradist * extradist / d1.sqAbs() );
	const Coord newc1( oldcrds[nrcoords-1].x + p * d1.x,
			   oldcrds[nrcoords-1].y + p * d1.y );

	coords += newc0;
	for ( int idx=0; idx<oldcrds.size(); idx++ )
	    coords += oldcrds[idx];
	coords += newc1;
    }
}


void uiWell2RandomLineDlg::previewPush( CallBacker* cb )
{
    wellserv_->sendPreviewEvent();
}


bool uiWell2RandomLineDlg::acceptOK( CallBacker* )
{
    if ( !outfld_->commitInput() || !outctio_.ioobj )
    {
	if ( outfld_->isEmpty() )
	    uiMSG().error( "Please specify the output" );
	return false;
    }

    Geometry::RandomLine* rl = new Geometry::RandomLine;
    TypeSet<Coord> wellcoord; getCoordinates( wellcoord );
    if ( wellcoord.size() < 2 )
    {
	uiMSG().error( "Please define at least two points" );
	return false;
    }

    for ( int idx=0; idx<wellcoord.size(); idx++ )
    {
	Coord c( wellcoord[idx] );
	if ( !SI().isInside(SI().transform(c),false) )
	{
	    Coord othcoord = wellcoord[idx ? idx - 1 : 1];
	    c = SurveyGeometry::getEdgePoint( othcoord, c );
	}
	rl->addNode( SI().transform(c) );
    }

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

