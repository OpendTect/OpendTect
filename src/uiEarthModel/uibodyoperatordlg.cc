/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Yuancheng Liu
 Date: 		Feb 2009
___________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uibodyoperatordlg.h"

#include "ctxtioobj.h"
#include "embodyoperator.h"
#include "embodytr.h"
#include "emmarchingcubessurface.h"
#include "emmanager.h"
#include "executor.h"
#include "ioman.h"
#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilistview.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiBodyOperatorDlg::uiBodyOperatorDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Body operation",mNoDlgTitle,mNoHelpID) )
{
    setCtrlStyle( DoAndStay );

    tree_ = new uiListView( this, "Operation tree", 9 );
    uiLabel* label0 = new uiLabel( this, "Operation tree" );
    label0->attach( centeredAbove, tree_ );
    tree_->setHScrollBarMode( uiListView::Auto );
    tree_->setVScrollBarMode( uiListView::Auto );
    tree_->setSelectionBehavior(uiListView::SelectRows);
    tree_->leftButtonClicked.notify( mCB(this,uiBodyOperatorDlg,itemClick) );

    BufferStringSet labels;
    labels.add( "Implicit body" );
    labels.add( "Action" );
    tree_->addColumns( labels );
    tree_->setColumnWidth( 0, 160 );
    tree_->setColumnWidth( 1, 30 );

    uiListViewItem* output = new uiListViewItem(tree_,uiListViewItem::Setup());
    output->setText( "Output body", 0 );
    output->setText( "Operator", 1 );
    output->setOpen( true );
    bodyOprand item = bodyOprand();
    item.defined = true;
    listinfo_ += item;
    listsaved_ += output;

    uiListViewItem* c0 = new uiListViewItem( output, uiListViewItem::Setup() );
    c0->setText( "input" );
    uiListViewItem* c1 = new uiListViewItem( output, uiListViewItem::Setup() );
    c1->setText( "input" );
    listinfo_ += bodyOprand();
    listinfo_ += bodyOprand();
    listsaved_ += c0;
    listsaved_ += c1;
   
    BufferStringSet btype;
    btype.add( "Stored body" );
    btype.add( "Operator" ); 
    typefld_ = new uiLabeledComboBox( this, btype, "Input type" );
    typefld_->attach( rightOf, tree_ );
    typefld_->box()->selectionChanged.notify(
	    mCB(this,uiBodyOperatorDlg,typeSel) ); 
    uiLabel* label1 = new uiLabel( this, "Operands" );
    label1->attach( centeredAbove, typefld_ );

    bodyselfld_ = new uiGenInput( this, "Input", StringInpSpec() );
    bodyselfld_->attach( alignedBelow, typefld_ );
    bodyselbut_ = new uiPushButton( this, "&Select", false );
    bodyselbut_->attach( rightOf, bodyselfld_ );
    bodyselbut_->activated.notify( mCB(this,uiBodyOperatorDlg,bodySel) );
    
    BufferStringSet operators;
    operators.add( "Union" );
    operators.add( "Intersection" );
    operators.add( "Minus" );
    oprselfld_ = new uiLabeledComboBox( this, operators, "Operator" );
    oprselfld_->attach( alignedBelow, typefld_ );
    oprselfld_->box()->selectionChanged.notify(
	    mCB(this,uiBodyOperatorDlg,oprSel) );

    unionbut_ = new uiToolButton( this, "set_union.png", "Union", CallBack() );
    unionbut_->attach( rightOf, oprselfld_ );
    intersectbut_ = new uiToolButton( this, "set_intersect.png", "Intersect",
	    				CallBack() );
    intersectbut_->attach( rightOf, oprselfld_ );
    minusbut_ = new uiToolButton( this, "set_minus.png", "Minus", CallBack() );
    minusbut_->attach( rightOf, oprselfld_ );

    outputfld_ = new uiIOObjSel( this, mIOObjContext(EMBody), "Output body" );
    outputfld_->setForRead( false );
    outputfld_->attach( alignedBelow, tree_ );

    typefld_->display( false );
    turnOffAll();
}


uiBodyOperatorDlg::~uiBodyOperatorDlg()
{ listinfo_.erase(); }


void uiBodyOperatorDlg::turnOffAll()
{
    bodyselfld_->display( false );
    bodyselbut_->display( false );

    oprselfld_->display( false );
    unionbut_->display( false );
    minusbut_->display( false );
    intersectbut_->display( false );
}


#define mDisplyAction( item, curidx ) \
    oprselfld_->box()->setCurrentItem( item==sKeyUdf() ? 0 : item ); \
    if ( item==sKeyIntSect() ) \
    { \
 	intersectbut_->display( true ); \
	listinfo_[curidx].act = sKeyIntSect(); \
	tree_->selectedItem()->setText( "Intersection", 1 ); \
	tree_->selectedItem()->setPixmap( 1, "set_intersect.png" ); \
    } \
    else if ( item==sKeyMinus() )  \
    { \
	minusbut_->display( true ); \
	listinfo_[curidx].act = sKeyMinus(); \
	tree_->selectedItem()->setText( "Minus", 1 ); \
	tree_->selectedItem()->setPixmap( 1, "set_minus.png" ); \
    } \
    else \
    { \
	unionbut_->display( true ); \
	listinfo_[curidx].act = sKeyUnion(); \
	tree_->selectedItem()->setText( "Union", 1 ); \
	tree_->selectedItem()->setPixmap( 1, "set_union.png" ); \
    } 


void uiBodyOperatorDlg::typeSel( CallBacker* cb )
{
    const bool isbodyitem = typefld_->box()->currentItem()==0;
    uiListViewItem* cur = tree_->selectedItem();
    const int curidx = listsaved_.indexOf( cur );
    
    if ( !isbodyitem )
    {
	turnOffAll();
	oprselfld_->display( true );
	mDisplyAction( listinfo_[curidx].act, curidx );
	listinfo_[curidx].defined = true;
	if ( tree_->selectedItem()->nrChildren() )
	    return;

	uiListViewItem* c0 = new uiListViewItem(cur,uiListViewItem::Setup());
	c0->setText( "input" );
	uiListViewItem* c1 = new uiListViewItem(cur,uiListViewItem::Setup());
	c1->setText( "input" );

	cur->setOpen( true );

	listinfo_ += bodyOprand();
	listinfo_ += bodyOprand();
	listsaved_ += c0;
	listsaved_ += c1;
    }
    else
    {
	if ( cur->nrChildren() )
	{
	    for ( int cid=cur->nrChildren()-1; cid>=0; cid-- )
	    {
		deleteAllChildInfo( cur->getChild(cid) );
		cur->removeItem( cur->getChild(cid) );
	    }

	    listinfo_[curidx].act = -1;
	    listinfo_[curidx].defined = false;

	    delete cur;
	    cur = new uiListViewItem( cur->parent(), uiListViewItem::Setup() );
	    cur->setText( "input" );
	}

	itemClick( cb );
    }
}


void uiBodyOperatorDlg::deleteAllChildInfo( uiListViewItem* curitem )
{
    if ( !curitem->nrChildren() )
    {
	int idx = listsaved_.indexOf( curitem );
	if ( idx==-1 )
	{
	    pErrMsg("Hmm"); return;
	}

	listsaved_.remove( idx );
	listsaved_.remove( idx );
	listinfo_.remove( idx );
	listinfo_.remove( idx );
	return;
    }

    for ( int cid=curitem->nrChildren()-1; cid>=0; cid-- )
	deleteAllChildInfo( curitem->getChild(cid) );
}


void uiBodyOperatorDlg::oprSel( CallBacker* )
{
    turnOffAll();
    oprselfld_->display( true );
  
    const int item = oprselfld_->box()->currentItem();
    const int curidx = listsaved_.indexOf( tree_->selectedItem() );
    listinfo_[curidx].defined = true;

    mDisplyAction( item, curidx );
}


void uiBodyOperatorDlg::itemClick( CallBacker* cb )
{
    typefld_->display( true );
    const int curidx = listsaved_.indexOf( tree_->selectedItem() );
    const char item = listinfo_[curidx].act!=sKeyUdf() ? listinfo_[curidx].act 
						       : sKeyUnion();
    typefld_->setSensitive( tree_->firstItem()!=tree_->selectedItem() );
    if ( !tree_->selectedItem()->nrChildren() )
    {
	turnOffAll();

    	bodyselfld_->display( true );
    	bodyselbut_->display( true );
	typefld_->box()->setCurrentItem( 0 );
	bodyselfld_->setText( listinfo_[curidx].defined ? 
		tree_->selectedItem()->text() : "" );
    }
    else 
    {
	turnOffAll();
    	oprselfld_->display( true );
	mDisplyAction( item, curidx );
	typefld_->box()->setCurrentItem( 1 );
    }
}


void uiBodyOperatorDlg::bodySel( CallBacker* )
{
    CtxtIOObj context( EMBodyTranslatorGroup::ioContext() );
    context.ctxt.forread = true;
    
    uiIOObjSelDlg dlg( parent(), context );
    if ( !dlg.go() || !dlg.ioObj() )
	return;
    
    tree_->selectedItem()->setText( dlg.ioObj()->name() );
    bodyselfld_->setText( dlg.ioObj()->name() );

    const int curidx = listsaved_.indexOf( tree_->selectedItem() );
    listinfo_[curidx].mid = dlg.ioObj()->key();
    listinfo_[curidx].defined = true;
}


#define mRetErr(msg) { uiMSG().error(msg); return false; }


bool uiBodyOperatorDlg::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<listinfo_.size(); idx++ )
    {
	if ( !listinfo_[idx].defined ||
	    (!listinfo_[idx].mid.isEmpty() && listinfo_[idx].act!=sKeyUdf())
	    || (listinfo_[idx].mid.isEmpty() && listinfo_[idx].act==sKeyUdf()))
	    mRetErr("Do not forget to pick Action/Body")
    }

    if ( outputfld_->isEmpty() )
	mRetErr("Select an output name")
    
    if ( !outputfld_->commitInput() )
	return false;
    
    RefMan<EM::MarchingCubesSurface> emcs = 
	new EM::MarchingCubesSurface(EM::EMM());
    if ( !emcs->getBodyOperator() )
	emcs->createBodyOperator();

    setOprator( listsaved_[0], *emcs->getBodyOperator() );
    if ( !emcs->getBodyOperator()->isOK() )
	mRetErr("Your operator is wrong")
    
    MouseCursorChanger bodyopration( MouseCursor::Wait );
    uiTaskRunner taskrunner( this );
    if ( !emcs->regenerateMCBody( &taskrunner ) )
	mRetErr("Generating body failed")

    emcs->setMultiID( outputfld_->key() );
    emcs->setName( outputfld_->getInput() );
    emcs->setFullyLoaded( true );
    emcs->setChangedFlag();

    EM::EMM().addObject( emcs );
    PtrMan<Executor> exec = emcs->saver();
    if ( !exec )
	mRetErr("Body saving failed")
	    
    MultiID key = emcs->multiID();
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj->pars().find( sKey::Type ) )
    {
	ioobj->pars().set( sKey::Type, emcs->getTypeStr() );
	if ( !IOM().commitChanges( *ioobj ) )
	    mRetErr("Writing body to disk failed, no permision?")
    }

    taskrunner.execute( *exec );
    
    BufferString msg = "The body ";
    msg += outputfld_->getInput();
    msg += " created successfully";
    uiMSG().message( msg.buf() );

    return false;
}


void uiBodyOperatorDlg::setOprator( uiListViewItem* lv, EM::BodyOperator& opt )
{
    if ( !lv || !lv->nrChildren() ) return;

    const int lvidx = listsaved_.indexOf( lv );
    if ( listinfo_[lvidx].act==sKeyUnion() ) 
	opt.setAction( EM::BodyOperator::Union ); 
    else if ( listinfo_[lvidx].act==sKeyIntSect() ) 
	opt.setAction( EM::BodyOperator::IntSect ); 
    else if ( listinfo_[lvidx].act==sKeyMinus() ) 
	opt.setAction( EM::BodyOperator::Minus ); 

    for ( int idx=0; idx<2; idx++ )
    {
	uiListViewItem* child = !idx ? lv->firstChild() : lv->lastChild();
	if ( child->nrChildren() )
	{
	    EM::BodyOperator* childoprt = new EM::BodyOperator();
	    opt.setInput( idx==0, childoprt );
	    setOprator( child, *childoprt );
	}
	else 
	{
	    const int chilidx = listsaved_.indexOf( child );
	    opt.setInput( idx==0, listinfo_[chilidx].mid );
	}
    }
}


uiBodyOperatorDlg::bodyOprand::bodyOprand()
{
    defined = false;  
    mid = 0;
    act = sKeyUdf();
}


bool uiBodyOperatorDlg::bodyOprand::operator==( const bodyOprand& v ) const
{ return mid==v.mid && act==v.act; }
