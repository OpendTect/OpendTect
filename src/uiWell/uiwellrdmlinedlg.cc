/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellrdmlinedlg.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survgeometry.h"
#include "survinfo.h"
#include "transl.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uiwellpartserv.h"
#include "welldata.h"
#include "wellman.h"
#include "welltrack.h"
#include "welltransl.h"

#include <math.h>


static const char* sTypes[] = { "Top", "Bottom", 0 };

uiWellSelGrp::uiWellSelGrp( uiParent* p, bool withpos )
    : uiGroup(p, "Select wells in table" )
    , orderChanged(this)
    , withpos_(withpos)
{
    auto* selbuttons = new uiGroup( this, "select buttons" );
    auto* movebuttons = new uiGroup( this, "move buttons" );

    createFields();
    createSelectButtons( selbuttons );
    createMoveButtons( movebuttons );

    attachFields( selbuttons, movebuttons );

    fillListBox();
    ptsSel( nullptr );
}


uiWellSelGrp::~uiWellSelGrp()
{}


void uiWellSelGrp::createFields()
{
    wellsbox_ = new uiListBox( this, "Available Wells", OD::ChooseZeroOrMore );
    selwellstbl_ = new uiTable( this, uiTable::Setup()
				        .selmode(uiTable::SelectionMode(3) ),
				"Wells Table" );
    selwellstbl_->setNrCols( withpos_ ? 2 : 1 );
    selwellstbl_->setNrRows( 5 );
    selwellstbl_->setColumnLabel( 0, mJoinUiStrs(sWell(),sName()) );
    selwellstbl_->setColumnWidth(0,90);
    if ( withpos_ )
    {
	selwellstbl_->setColumnLabel( 1, tr("Start at") );
	selwellstbl_->setColumnWidth(1,90);

	onlytopfld_ = new uiGenInput( this, tr("Use only wells' top position"),
				      BoolInpSpec(true) );
	onlytopfld_->valueChanged.notify( mCB(this,uiWellSelGrp,ptsSel) );
	onlytopfld_->attach( alignedBelow, selwellstbl_ );
	onlytopfld_->attach( ensureBelow, wellsbox_ );
    }

    selwellstbl_->setTableReadOnly(true);
    setHAlignObj( selwellstbl_ );
}


void uiWellSelGrp::createSelectButtons( uiGroup* selbuttons )
{
    auto* sellbl = new uiLabel( selbuttons, uiStrings::sSelect() );
    CallBack cb = mCB(this,uiWellSelGrp,selButPush);
    toselect_ = new uiToolButton( selbuttons, uiToolButton::RightArrow,
					tr("Move right"), cb );
    toselect_->attach( centeredBelow, sellbl );
    toselect_->setHSzPol( uiObject::Undef );
    fromselect_ = new uiToolButton( selbuttons, uiToolButton::LeftArrow,
					tr("Move left"), cb );
    fromselect_->attach( alignedBelow, toselect_ );
    fromselect_->setHSzPol( uiObject::Undef );
    selbuttons->setHAlignObj( toselect_ );
}


void uiWellSelGrp::createMoveButtons( uiGroup* movebuttons )
{
    auto* movelbl = new uiLabel( movebuttons, tr("Change \n order") );
    CallBack cb = mCB(this,uiWellSelGrp,moveButPush);
    moveupward_ = new uiToolButton( movebuttons, uiToolButton::UpArrow,
				    tr("Move Up"), cb );
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::Undef );
    movedownward_ = new uiToolButton( movebuttons, uiToolButton::DownArrow,
					tr("Move Down"), cb );
    movedownward_->attach( alignedBelow, moveupward_ );
    movedownward_->setHSzPol( uiObject::Undef );
    movebuttons->setHAlignObj( moveupward_ );
}


void uiWellSelGrp::attachFields( uiGroup* selbuttons, uiGroup* movebuttons )
{
    selbuttons->attach( centeredLeftOf, selwellstbl_ );
    selbuttons->attach( ensureRightOf, wellsbox_ );
    selwellstbl_->attach( rightTo, wellsbox_ );
    movebuttons->attach( centeredRightOf, selwellstbl_ );
    if ( onlytopfld_ )
	onlytopfld_->attach( ensureBelow, selwellstbl_ );
}


void uiWellSelGrp::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( but == toselect_ )
    {
	int lastusedidx = 0;
	for ( int idx=0; idx<wellsbox_->size(); idx++ )
	{
	    if ( !wellsbox_->isChosen(idx) ) continue;

	    int emptyrow = getFirstEmptyRow();
	    if ( emptyrow == -1 )
	    {
		emptyrow = selwellstbl_->nrRows();
		selwellstbl_->insertRows( selwellstbl_->nrRows(), 1 );
	    }
	    selwellstbl_->setText( RowCol(emptyrow,0),
				   wellsbox_->textOfItem(idx));
	    auto* box = new uiComboBox( 0, sTypes, "Type" );
	    selwellstbl_->setCellObject( RowCol(emptyrow,1), box );
	    box->setValue( 0 );
	    wellsbox_->removeItem(idx);
	    lastusedidx = idx;
	    idx--;
	}
	wellsbox_->sortItems();
	wellsbox_->setCurrentItem( lastusedidx<wellsbox_->size() ?
				   lastusedidx : wellsbox_->size()-1 );
	int selectidx = getFirstEmptyRow()-1<0 ?
			selwellstbl_->nrRows()-1 : getFirstEmptyRow()-1;
	selwellstbl_->selectRow( selectidx );
    }
    else if ( but == fromselect_ )
    {
	int lastusedidx = 0;
	for ( int idx=0; idx<selwellstbl_->nrRows(); idx++ )
	{
	    if ( !selwellstbl_->isRowSelected(idx)
		|| StringView(selwellstbl_->text(RowCol(idx,0))).isEmpty() )
		continue;

	    wellsbox_->addItem(toUiString(selwellstbl_->text(RowCol(idx,0))) );
	    selwellstbl_->removeRow(idx);
	    lastusedidx = idx;
	    wellsbox_->chooseAll(false);
	    wellsbox_->setChosen( wellsbox_->size()-1 );
	    wellsbox_->sortItems();
	    break;
	}
	while ( selwellstbl_->nrRows() < 5 )
	    selwellstbl_->insertRows( selwellstbl_->nrRows(), 1 );

	int selectidx;

	if ( lastusedidx<1 )
	    selectidx = 0;
	else if ( lastusedidx<selwellstbl_->nrRows() )
	    selectidx = lastusedidx-1;
	else
	    selectidx = selwellstbl_->nrRows()-1;

	selwellstbl_->selectRow( selectidx );
    }

    for ( int idx=0; idx<selwellstbl_->nrRows(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,box,
			selwellstbl_->getCellObject(RowCol(idx,1)))
	if ( box ) box->setName( BufferString("Type",idx) );
    }

    ptsSel( nullptr );
}


void uiWellSelGrp::fillListBox()
{
    IOObjContext ctxt = mIOObjContext( Well );
    ctxt.forread_ = true;
    const IODir iodir ( ctxt.getSelKey() );
    const IODirEntryList entrylist( iodir, ctxt );
    for ( const auto* entry : entrylist )
    {
	if ( !entry || !entry->ioobj_ )
	    continue;

	allwellsids_.add( entry->ioobj_->key() );
	allwellsnames_.add( entry->name() );
    }

    wellsbox_->addItems( allwellsnames_ );
}


void uiWellSelGrp::setSelectedWells()
{
    selwellsids_.erase();
    selwellstypes_.erase();

    for ( int idx=0; idx<selwellstbl_->nrRows(); idx++ )
    {
	const char* txt = selwellstbl_->text( RowCol(idx,0) );
	int wellidx = allwellsnames_.indexOf( txt );
	if ( wellidx<0 ) continue;
	if ( selwellsids_.addIfNew( allwellsids_[wellidx] ) )
	{
	    mDynamicCastGet(uiComboBox*,box,
			    selwellstbl_->getCellObject(RowCol(idx,1)))
	    if ( box )
		selwellstypes_ += box->currentItem();
	}
    }
}


void uiWellSelGrp::getCoordinates( TypeSet<Coord>& coords )
{
    setSelectedWells();
    const bool onlytop = onlytopfld_ ? onlytopfld_->getBoolValue() : false;
    for ( int idx=0; idx<selwellsids_.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = Well::MGR().get( selwellsids_[idx] );
	if ( !wd || wd->track().isEmpty() )
	    return;

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
	selwellstbl_->insertRows( rowidx, 1 );\
	selwellstbl_->setText( RowCol(rowidx,0), text );\
	auto* newbox = new uiComboBox(0,sTypes,"Type"); \
	newbox->setValue( val ); \
	selwellstbl_->setCellObject( RowCol(rowidx,1), newbox );

void uiWellSelGrp::moveButPush( CallBacker* cb )
{
    int index = selwellstbl_->currentRow();
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( !selwellstbl_->isRowSelected(index) )
	    return;

    if ( StringView(selwellstbl_->text(RowCol(index,0))).isEmpty() )
	return;

    mDynamicCastGet(uiComboBox*,box,
		    selwellstbl_->getCellObject(RowCol(index,1)))
    const int value = box ? box->getIntValue(): 0;
    BufferString text = selwellstbl_->text( RowCol(index,0) );

    if ( but == moveupward_ && index>0 )
    {
	mInsertRow( index-1, text.buf(), value );
	selwellstbl_->removeRow( index+1 );
	selwellstbl_->selectRow( index-1 );
	selwellstbl_->setCurrentCell( RowCol(index-1,0) );
    }
    else if ( but == movedownward_ && index<selwellstbl_->nrRows()
	      && !StringView(selwellstbl_->text(RowCol(index+1,0))).isEmpty() )
    {
	mInsertRow( index+2, text.buf(), value );
	selwellstbl_->removeRow( index );
	selwellstbl_->selectRow( index+1 );
	selwellstbl_->setCurrentCell( RowCol(index+1,0) );
    }

    for ( int idx=0; idx<selwellstbl_->nrRows(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,wellbox,
			selwellstbl_->getCellObject(RowCol(idx,1)))
	if ( wellbox )
		wellbox->setName( BufferString("Type",idx) );
    }

    ptsSel( nullptr );
}


int uiWellSelGrp::getFirstEmptyRow()
{
    for ( int idx=0; idx<selwellstbl_->nrRows(); idx++ )
    {
	if ( StringView(selwellstbl_->text(RowCol(idx,0))).isEmpty() )
	    return idx;
    }

    return -1;
}


void uiWellSelGrp::ptsSel( CallBacker* )
{
    selwellstbl_->hideColumn( 1, onlytopfld_->getBoolValue() );
    orderChanged.trigger();
}




uiWell2RandomLineDlg::uiWell2RandomLineDlg( uiParent* p, uiWellPartServer* ws )
    : uiDialog(p,Setup(tr("Create Random line"),
		       tr("Select wells to set up the random line path"),
		       mODHelpKey(mWell2RandomLineDlgHelpID)).modal(false))
    , wellserv_(ws)
{
    selgrp_ = new uiWellSelGrp( this );
    mAttachCB( selgrp_->orderChanged, uiWell2RandomLineDlg::previewPush );

    createFields();
    attachFields();
}


uiWell2RandomLineDlg::~uiWell2RandomLineDlg()
{
    detachAllNotifiers();
}


void uiWell2RandomLineDlg::createFields()
{
    uiString txt = tr( "Extend outward %1" ).arg( SI().getUiXYUnitString() );
    const int defdist = mNINT32( 100 * SI().inlDistance() );
    extendfld_ = new uiGenInput(this,txt,FloatInpSpec(float(defdist)));
    extendfld_->setWithCheck( true );
    extendfld_->setChecked( true );

    auto* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, extendfld_ );

    IOObjContext ctxt = mIOObjContext( RandomLineSet );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			     uiStrings::phrOutput( uiStrings::sRandomLine() ) );
    if ( wellserv_ )
    {
	CallBack cb = mCB(this,uiWell2RandomLineDlg,previewPush);
	previewbutton_ = new uiPushButton( this, tr("Preview"), cb, true );
	previewbutton_->attach( ensureBelow, sep );
	dispfld_ = new uiCheckBox( this,
				   tr("Display Random Line on creation") );
	dispfld_->setChecked( true );
    }
}


void uiWell2RandomLineDlg::attachFields()
{
    extendfld_->attach( alignedBelow, selgrp_ );
    if ( !previewbutton_ )
	outfld_->attach( alignedBelow, extendfld_ );
    else
    {
	previewbutton_->attach( alignedBelow, extendfld_ );
	outfld_->attach( alignedBelow, previewbutton_ );
	dispfld_->attach( alignedBelow, outfld_ );
    }
}


bool uiWell2RandomLineDlg::dispOnCreation()
{
    return dispfld_ ? dispfld_->isChecked() : false;
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
    float extradist = extendfld_->getFValue();
    if ( extradist < 0.1 || extradist > 1e6 ) return;
    if ( nrcoords == 1 )
    {
	const Coord c( coords[0] );
	coords.erase();
        coords += Coord( c.x_-extradist, c.y_ );
	coords += c;
        coords += Coord( c.x_+extradist, c.y_ );
    }
    else
    {
	TypeSet<Coord> oldcrds( coords );
	coords.erase();
        const Coord d0( oldcrds[1].x_ - oldcrds[0].x_,
                oldcrds[1].y_ - oldcrds[0].y_ );
	float p = (float) Math::Sqrt( extradist * extradist / d0.sqAbs() );
	const Coord newc0( oldcrds[0].x_ - p * d0.x_,
			   oldcrds[0].y_ - p * d0.y_ );
        const Coord d1( oldcrds[nrcoords-1].x_ - oldcrds[nrcoords-2].x_,
                oldcrds[nrcoords-1].y_ - oldcrds[nrcoords-2].y_ );
	p = (float) Math::Sqrt( extradist * extradist / d1.sqAbs() );
        const Coord newc1( oldcrds[nrcoords-1].x_ + p * d1.x_,
                oldcrds[nrcoords-1].y_ + p * d1.y_ );

	coords += newc0;
	for ( int idx=0; idx<oldcrds.size(); idx++ )
	    coords += oldcrds[idx];
	coords += newc1;
    }
}


void uiWell2RandomLineDlg::previewPush( CallBacker* )
{
    if ( wellserv_ )
	wellserv_->sendPreviewEvent();
}


bool uiWell2RandomLineDlg::acceptOK( CallBacker* )
{
    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	return false;

    TypeSet<Coord> wellcoord; getCoordinates( wellcoord );
    if ( wellcoord.size() < 2 )
    {
	uiMSG().error( tr("Please define at least two points") );
	return false;
    }

    RefMan<Geometry::RandomLine> rl = new Geometry::RandomLine;
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
    outrls.addLine( *rl );
    uiString errmsg;
    const bool res = RandomLineSetTranslator::store( outrls, ioobj, errmsg );
    if ( !res )
	uiMSG().error( errmsg );

    return res;
}


MultiID uiWell2RandomLineDlg::getRandLineID() const
{
    return outfld_->key( true );
}
