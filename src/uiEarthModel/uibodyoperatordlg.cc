/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibodyoperatordlg.h"

#include "ctxtioobj.h"
#include "embodyoperator.h"
#include "embodytr.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "marchingcubes.h"
#include "od_helpids.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uiiosurface.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uitreeview.h"


uiBodyOperatorDlg::uiBodyOperatorDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Apply Geobody Operations"),mNoDlgTitle,
				mODHelpKey(mBodyOperatorDlgHelpID) ) )
{
    setCtrlStyle( RunAndClose );

    uiGroup* lgrp = new uiGroup( this, "Left Group" );
    tree_ = new uiTreeView( lgrp, "Operation tree", 9 );
    uiLabel* label0 = new uiLabel( lgrp, tr("Operation tree") );
    label0->attach( centeredAbove, tree_ );
    tree_->setHScrollBarMode( uiTreeView::Auto );
    tree_->setVScrollBarMode( uiTreeView::Auto );
    tree_->setSelectionBehavior(uiTreeView::SelectRows);
    tree_->leftButtonClicked.notify( mCB(this,uiBodyOperatorDlg,itemClick) );

    BufferStringSet labels;
    labels.add( "Implicit body" );
    labels.add( "Action" );
    tree_->addColumns( labels );
    tree_->setColumnWidthMode( 1, uiTreeView::ResizeToContents );

    uiTreeViewItem* output = new uiTreeViewItem(tree_,uiTreeViewItem::Setup());
    output->setText( uiStrings::phrOutput( tr("body") ), 0 );
    output->setText( uiStrings::sOperator(), 1 );
    output->setOpen( true );
    BodyOperand item = BodyOperand();
    item.defined_ = true;
    listinfo_ += item;
    listsaved_ += output;

    uiTreeViewItem* c0 = new uiTreeViewItem( output, uiTreeViewItem::Setup() );
    c0->setText( uiStrings::sInput() );
    uiTreeViewItem* c1 = new uiTreeViewItem( output, uiTreeViewItem::Setup() );
    c1->setText( uiStrings::sInput() );
    listinfo_ += BodyOperand();
    listinfo_ += BodyOperand();
    listsaved_ += c0;
    listsaved_ += c1;

    uiGroup* rgrp = new uiGroup( this, "Right Group" );
    BufferStringSet btype;
    btype.add( "Stored geobody" );
    btype.add( "Operator" );
    typefld_ = new uiLabeledComboBox( rgrp, btype, tr("Input type") );
    typefld_->box()->selectionChanged.notify(
	    mCB(this,uiBodyOperatorDlg,typeSel) );
    uiLabel* label1 = new uiLabel( rgrp, tr("Operands") );
    label1->attach( centeredAbove, typefld_ );

    bodyselfld_ = new uiGenInput( rgrp, uiStrings::sInput(), StringInpSpec() );
    bodyselfld_->attach( alignedBelow, typefld_ );
    bodyselbut_ = new uiPushButton( rgrp, uiStrings::sSelect(), false );
    bodyselbut_->attach( rightOf, bodyselfld_ );
    bodyselbut_->activated.notify( mCB(this,uiBodyOperatorDlg,bodySel) );

    BufferStringSet operators;
    operators.add( "Union" ).add( "Intersection" ).add( "Difference" );
    oprselfld_ = new uiLabeledComboBox( rgrp, operators,
					uiStrings::sOperator() );
    oprselfld_->box()->setIcon( 0, "set_union" );
    oprselfld_->box()->setIcon( 1, "set_intersect" );
    oprselfld_->box()->setIcon( 2, "set_minus" );
    oprselfld_->attach( alignedBelow, typefld_ );
    oprselfld_->box()->selectionChanged.notify(
	    mCB(this,uiBodyOperatorDlg,oprSel) );

    outputfld_ = new uiBodySel( lgrp, false );
    outputfld_->setHSzPol( uiObject::MedVar );
    outputfld_->attach( leftAlignedBelow, tree_ );

    rgrp->attach( rightTo, lgrp );

    typefld_->display( false );
    turnOffAll();

    postFinalize().notify( mCB(this,uiBodyOperatorDlg,finalizeCB) );
}


uiBodyOperatorDlg::~uiBodyOperatorDlg()
{
    listinfo_.erase();
}


MultiID uiBodyOperatorDlg::getBodyMid() const
{
    return outputfld_->key();
}


void uiBodyOperatorDlg::finalizeCB( CallBacker* )
{
    tree_->setSelected( tree_->firstItem(), true );
    tree_->setCurrentItem( tree_->firstItem(), 0 );
    itemClick(0);
}


void uiBodyOperatorDlg::turnOffAll()
{
    bodyselfld_->display( false );
    bodyselbut_->display( false );
    oprselfld_->display( false );
}


void uiBodyOperatorDlg::displayAction( char item, int curidx )
{
    oprselfld_->box()->setCurrentItem( item==sKeyUdf() ? 0 : item );
    if ( item==sKeyIntSect() )
    {
	listinfo_[curidx].act_ = sKeyIntSect();
	tree_->selectedItem()->setText( tr("Intersection"), 1 );
	tree_->selectedItem()->setIcon( 1, "set_intersect" );
    }
    else if ( item==sKeyMinus() )
    {
	listinfo_[curidx].act_ = sKeyMinus();
	tree_->selectedItem()->setText( tr("Difference"), 1 );
	tree_->selectedItem()->setIcon( 1, "set_minus" );
    }
    else
    {
	listinfo_[curidx].act_ = sKeyUnion();
	tree_->selectedItem()->setText( tr("Union"), 1 );
	tree_->selectedItem()->setIcon( 1, "set_union" );
    }
}


void uiBodyOperatorDlg::typeSel( CallBacker* cb )
{
    const bool isbodyitem = typefld_->box()->currentItem()==0;
    uiTreeViewItem* cur = tree_->selectedItem();
    const int curidx = listsaved_.indexOf( cur );
    if ( !listinfo_.validIdx(curidx) )
	return;

    if ( !isbodyitem )
    {
	turnOffAll();
	oprselfld_->display( true );
	displayAction( listinfo_[curidx].act_, curidx );
	listinfo_[curidx].defined_ = true;
	if ( tree_->selectedItem()->nrChildren() )
	    return;

	uiTreeViewItem* c0 = new uiTreeViewItem(cur,uiTreeViewItem::Setup());
	c0->setText( uiStrings::sInput() );
	uiTreeViewItem* c1 = new uiTreeViewItem(cur,uiTreeViewItem::Setup());
	c1->setText( uiStrings::sInput() );

	cur->setOpen( true );

	listinfo_ += BodyOperand();
	listinfo_ += BodyOperand();
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

	    listinfo_[curidx].act_ = -1;
	    listinfo_[curidx].defined_ = false;

	    tree_->selectedItem()->setText( uiString::emptyString(), 1 );
	    tree_->selectedItem()->setIcon( 1, "empty" );
	}

	itemClick( cb );
    }
}


void uiBodyOperatorDlg::deleteAllChildInfo( uiTreeViewItem* curitem )
{
    if ( !curitem->nrChildren() )
    {
	int idx = listsaved_.indexOf( curitem );
	if ( idx==-1 )
	{
	    pErrMsg("Hmm"); return;
	}

	listsaved_.removeSingle( idx );
	listinfo_.removeSingle( idx );
    }
    else
    {
	for ( int cid=curitem->nrChildren()-1; cid>=0; cid-- )
	    deleteAllChildInfo( curitem->getChild(cid) );
    }
}


void uiBodyOperatorDlg::oprSel( CallBacker* )
{
    turnOffAll();
    oprselfld_->display( true );

    const int item = oprselfld_->box()->currentItem();
    const int curidx = listsaved_.indexOf( tree_->selectedItem() );
    listinfo_[curidx].defined_ = true;

    displayAction( (char) item, curidx );
}


void uiBodyOperatorDlg::itemClick( CallBacker* )
{
    typefld_->display( true );
    const int curidx = listsaved_.indexOf( tree_->selectedItem() );
    if ( !listinfo_.validIdx(curidx) )
	return;

    const char item = listinfo_[curidx].act_!=sKeyUdf() ? listinfo_[curidx].act_
						       : sKeyUnion();
    typefld_->setSensitive( tree_->firstItem()!=tree_->selectedItem() );
    if ( !tree_->selectedItem()->nrChildren() )
    {
	turnOffAll();

	bodyselfld_->display( true );
	bodyselbut_->display( true );
	typefld_->box()->setCurrentItem( 0 );
	PtrMan<IOObj> ioobj = IOM().get( listinfo_[curidx].mid_ );
	const BufferString text = ioobj ? ioobj->name().buf() : "";
	bodyselfld_->setText( text );
    }
    else
    {
	turnOffAll();
	oprselfld_->display( true );
	displayAction( item, curidx );
	typefld_->box()->setCurrentItem( 1 );
    }
}


void uiBodyOperatorDlg::bodySel( CallBacker* )
{
    CtxtIOObj context( EMBodyTranslatorGroup::ioContext() );
    context.ctxt_.forread_ = true;

    uiIOObjSelDlg dlg( parent(), context );
    if ( !dlg.go() || !dlg.ioObj() )
	return;

    tree_->selectedItem()->setText( dlg.ioObj()->uiName() );
    bodyselfld_->setText( dlg.ioObj()->name() );

    const int curidx = listsaved_.indexOf( tree_->selectedItem() );
    listinfo_[curidx].mid_ = dlg.ioObj()->key();
    listinfo_[curidx].defined_ = true;
}


#define mRetErr(msg) { uiMSG().error(msg); return false; }


bool uiBodyOperatorDlg::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<listinfo_.size(); idx++ )
    {
	if ( !listinfo_[idx].isOK() )
	    mRetErr(tr("Do not forget to pick action"))
    }

    if ( outputfld_->isEmpty() )
	mRetErr(tr("Select an output name"))

    if ( !outputfld_->commitInput() )
	return false;

    RefMan<EM::MarchingCubesSurface> emcs =
	new EM::MarchingCubesSurface(EM::EMM());
    if ( !emcs->getBodyOperator() )
	emcs->createBodyOperator();

    setOperator( listsaved_[0], *emcs->getBodyOperator() );
    if ( !emcs->getBodyOperator()->isOK() )
	mRetErr(tr("Your operator is wrong"))

    MouseCursorChanger bodyopration( MouseCursor::Wait );
    uiTaskRunner taskrunner( this );
    if ( !emcs->regenerateMCBody( &taskrunner ) )
	mRetErr(tr("Generating geobody failed"))

    emcs->setMultiID( outputfld_->key() );
    emcs->setName( outputfld_->getInput() );
    emcs->setFullyLoaded( true );
    emcs->setChangedFlag();

    EM::EMM().addObject( emcs );
    PtrMan<Executor> exec = emcs->saver();
    if ( !exec )
	mRetErr(uiStrings::sSaveBodyFail())

    MultiID key = emcs->multiID();
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj->pars().hasKey(sKey::Type()) )
    {
	ioobj->pars().set( sKey::Type(), emcs->getTypeStr() );
	if ( !IOM().commitChanges( *ioobj ) )
	    mRetErr( uiStrings::phrCannotWriteDBEntry( ioobj->uiName() ) )
    }

    TaskRunner::execute( &taskrunner, *exec );

    uiString msg = tr("The geobody %1 created successfully")
                 .arg(outputfld_->getInput());
    uiMSG().message( msg );

    return false;
}


void uiBodyOperatorDlg::setOperator( uiTreeViewItem* lv, EM::BodyOperator& opt )
{
    if ( !lv || !lv->nrChildren() ) return;

    const int lvidx = listsaved_.indexOf( lv );
    if ( listinfo_[lvidx].act_==sKeyUnion() )
	opt.setAction( EM::BodyOperator::Union );
    else if ( listinfo_[lvidx].act_==sKeyIntSect() )
	opt.setAction( EM::BodyOperator::IntSect );
    else if ( listinfo_[lvidx].act_==sKeyMinus() )
	opt.setAction( EM::BodyOperator::Minus );

    for ( int idx=0; idx<2; idx++ )
    {
        uiTreeViewItem* child = !idx ? lv->firstChild() : lv->lastChild();
	if ( child->nrChildren() )
	{
	    EM::BodyOperator* childoprt = new EM::BodyOperator();
	    opt.setInput( idx==0, childoprt );
	    setOperator( child, *childoprt );
	}
	else
	{
	    const int chilidx = listsaved_.indexOf( child );
	    opt.setInput( idx==0, listinfo_[chilidx].mid_ );
	}
    }
}


uiBodyOperatorDlg::BodyOperand::BodyOperand()
{
    defined_ = false;
    mid_.setUdf();
    act_ = sKeyUdf();
}


bool uiBodyOperatorDlg::BodyOperand::operator==( const BodyOperand& v ) const
{ return mid_==v.mid_ && act_==v.act_; }


bool uiBodyOperatorDlg::BodyOperand::isOK() const
{
    if ( !defined_ ) return false;

    return (!mid_.isUdf() && act_==sKeyUdf()) ||
	   (mid_.isUdf() && act_!=sKeyUdf());
}


//uiImplicitBodyValueSwitchDlg
uiImplicitBodyValueSwitchDlg::uiImplicitBodyValueSwitchDlg( uiParent* p,
	const IOObj* ioobj )
    : uiDialog(p,uiDialog::Setup(tr("Geobody conversion - inside-out"),
		mNoDlgTitle, mODHelpKey(mImplicitBodyValueSwitchDlgHelpID) ) )
{
    setCtrlStyle( RunAndClose );

    inputfld_ = new uiBodySel( this, true );
    if ( ioobj )
	inputfld_->setInput( *ioobj );

    outputfld_ = new uiBodySel( this, false );
    outputfld_->attach( alignedBelow, inputfld_ );
}


uiImplicitBodyValueSwitchDlg::~uiImplicitBodyValueSwitchDlg()
{}



MultiID uiImplicitBodyValueSwitchDlg::getBodyMid() const
{
    return outputfld_->key();
}


bool uiImplicitBodyValueSwitchDlg::acceptOK( CallBacker* )
{
    const IOObj* inpiobj = inputfld_->ioobj(true);
    if ( !inpiobj )
	inpiobj = getIfMCSurfaceObj();

    if ( !inpiobj || !outputfld_->ioobj() )
	return false;

    uiTaskRunner taskrunner( this );
    RefMan<EM::EMObject> emo =
	EM::EMM().loadIfNotFullyLoaded( inpiobj->key(), &taskrunner );
    mDynamicCastGet(EM::Body*,emb,emo.ptr());
    if ( !emb )
	mRetErr( uiStrings::phrCannotRead( uiStrings::sGeobody()) );

    PtrMan<EM::ImplicitBody> impbd =
				emb->createImplicitBody( &taskrunner, false );
    if ( !impbd || !impbd->arr_ )
	mRetErr( tr("Creating geobody failed") );

    float* data = impbd->arr_->getData();
    if ( data )
    {
	const od_int64 sz = impbd->arr_->info().getTotalSz();
	for ( od_int64 idx=0; idx<sz; idx++ )
	    data[idx] = -data[idx];
    }
    else
    {
	const int isz = impbd->arr_->info().getSize(0);
	const int csz = impbd->arr_->info().getSize(1);
	const int zsz = impbd->arr_->info().getSize(2);
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		for ( int idz=0; idz<zsz; idz++ )
		{
		    const float val = impbd->arr_->get( idx, idy, idz );
		    impbd->arr_->set( idx, idy, idz, -val );
		}
	    }
	}
    }

    RefMan<EM::MarchingCubesSurface> emcs =
	new EM::MarchingCubesSurface( EM::EMM() );

    emcs->surface().setVolumeData( 0, 0, 0, *impbd->arr_, 0, &taskrunner );
    emcs->setInlSampling( SamplingData<int>(impbd->tkzs_.hsamp_.inlRange()) );
    emcs->setCrlSampling( SamplingData<int>(impbd->tkzs_.hsamp_.crlRange()) );
    emcs->setZSampling( SamplingData<float>(impbd->tkzs_.zsamp_) );

    emcs->setMultiID( outputfld_->key() );
    emcs->setName( outputfld_->getInput() );
    emcs->setFullyLoaded( true );
    emcs->setChangedFlag();

    EM::EMM().addObject( emcs );
    PtrMan<Executor> exec = emcs->saver();
    if ( !exec )
	mRetErr( uiStrings::sSaveBodyFail() );

    PtrMan<IOObj> ioobj = IOM().get( outputfld_->key() );
    if ( !ioobj->pars().hasKey(sKey::Type()) )
    {
	ioobj->pars().set( sKey::Type(), emcs->getTypeStr() );
	if ( !IOM().commitChanges(*ioobj) )
	    mRetErr( uiStrings::phrCannotWriteDBEntry( ioobj->uiName() ) )
    }

    if ( !TaskRunner::execute(&taskrunner,*exec) )
	mRetErr(uiStrings::sSaveBodyFail());

    return true;
}


const IOObj* uiImplicitBodyValueSwitchDlg::getIfMCSurfaceObj() const
{
    const char* inpstr = inputfld_->getInput();
    const CtxtIOObj workctio = mIOObjContext( EMBody );
    const IODir iodir( workctio.ctxt_.getSelKey() );
    const IOObj* inpiobj = iodir.get( inpstr );
    if ( !inpiobj )
	return 0;

    const int res = workctio.ctxt_.trgroup_->objSelector( inpiobj->group() );
    if ( res == mObjSelUnrelated )
	return 0;

    return inpiobj->clone();
}
