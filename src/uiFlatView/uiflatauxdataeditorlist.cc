/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatauxdataeditorlist.cc,v 1.14 2010/11/10 15:26:43 cvsbert Exp $";

#include "uiflatauxdataeditorlist.h"

#include "flatauxdataeditor.h"
#include "uilistbox.h"


uiFlatViewAuxDataEditorList::uiFlatViewAuxDataEditorList( uiParent* p )
    : uiGroup( p )
    , change_( this )  
    , ptselchange_( this )
    , isrectangleselection_( true )
{
    listbox_ = new uiListBox( this );
    listbox_->setMultiSelect( true );
    listbox_->selectionChanged.notify(
	    mCB(this, uiFlatViewAuxDataEditorList, listSelChangeCB) );
    listbox_->rightButtonClicked.notify(
	    mCB(this, uiFlatViewAuxDataEditorList, rightClickedCB) );
    uimenuhandler_ = new uiMenuHandler( p, 0 );
    uimenuhandler_->ref();
}


uiFlatViewAuxDataEditorList::~uiFlatViewAuxDataEditorList()
{
    listbox_->selectionChanged.remove(
	    mCB(this, uiFlatViewAuxDataEditorList, listSelChangeCB) );

    listbox_->rightButtonClicked.remove(
	    mCB(this, uiFlatViewAuxDataEditorList, rightClickedCB) );

    uimenuhandler_->unRef();
}


void uiFlatViewAuxDataEditorList::addEditor( FlatView::AuxDataEditor* newed )
{
    newed->movementFinished.notify( mCB(this,uiFlatViewAuxDataEditorList,
					pointSelectionChangedCB) );
    editors_ += newed;
    newed->setSelectionPolygonRectangle( isrectangleselection_ );
    updateList();
}


void uiFlatViewAuxDataEditorList::removeEditor( FlatView::AuxDataEditor* ed )
{
    ed->movementFinished.remove( mCB(this,uiFlatViewAuxDataEditorList,
				     pointSelectionChangedCB) );
    editors_ -= ed;
    updateList();
}


void uiFlatViewAuxDataEditorList::updateList( CallBacker* )
{
    ObjectSet<FlatView::AuxDataEditor> selectededitors;
    TypeSet<int> selectedids;
    getSelections( selectededitors, selectedids );

    NotifyStopper block( listbox_->selectionChanged );

    listbox_->setEmpty();
    listboxeditors_.erase();
    listboxids_.erase();

    for ( int idx=editors_.size()-1; idx>=0; idx-- )
    {
	FlatView::AuxDataEditor& editor = *editors_[idx];
	const TypeSet<int>& ids = editor.getIds();

        const ObjectSet<FlatView::Annotation::AuxData>& auxdata =
	    editor.getAuxData();

	for ( int idy=0; idy<ids.size(); idy++ )
	{
	    const FlatView::Annotation::AuxData* ad = auxdata[idy];
	    if ( !ad->markerstyles_.size() ||
		 !ad->markerstyles_[0].color_.isVisible() ||
		 !ad->enabled_ || ad->name_.isEmpty() )
		continue;

	    listbox_->insertItem( ad->name_, ad->markerstyles_[0].color_,
				  listbox_->size() );

	    listboxids_ += ids[idy];
	    listboxeditors_ += editors_[idx];
	}
    }
    
    listbox_->selectAll( false );

    for ( int idy=selectededitors.size()-1; idy>=0; idy-- )
    {
	const int idx = findEditorIDPair( selectededitors[idy],
					  selectedids[idy] );
	if ( idx==-1 )
	    continue;

       listbox_->setSelected( idx, true );
    }

    if ( listbox_->size()==1 )
       listbox_->setSelected( 0, true );

    block.restore();
    listbox_->selectionChanged.trigger();
    change_.trigger();
}


void uiFlatViewAuxDataEditorList::pointSelectionChangedCB( CallBacker* cb )
{
    const int idx = editors_.indexOf( (FlatView::AuxDataEditor*) cb );

    if ( idx==-1 || editors_[idx]->getSelPtDataID()!=-1 ||
	            editors_[idx]->getSelPtIdx().size() )
	return;

    ptselchange_.trigger();
}



void uiFlatViewAuxDataEditorList:: rightClickedCB(CallBacker*)
{
    uimenuhandler_->executeMenu( uimenuhandler_->fromTree(), 0 );
}


void uiFlatViewAuxDataEditorList::getSelections( 
	ObjectSet<FlatView::AuxDataEditor>& editors, TypeSet<int>& ids ) 
{
    for ( int idx=listbox_->size()-1; idx >= 0; idx-- )
    {
	if ( listbox_->isSelected( idx ) )
	{
	    editors += listboxeditors_[idx];
	    ids += listboxids_[idx];
	 }
    }
}


void uiFlatViewAuxDataEditorList::setSelection( 
	const FlatView::AuxDataEditor* editor, int id )
{
    const int idx = findEditorIDPair( editor, id );
    if ( idx<0 ) return;

    NotifyStopper block( listbox_->selectionChanged );
    listbox_->selectAll( false );
    block.restore();

    listbox_->setSelected( idx, true );
    change_.trigger();
}


bool uiFlatViewAuxDataEditorList::isRectangleSelection() const
{ return isrectangleselection_; }


void uiFlatViewAuxDataEditorList::useRectangleSelection(bool yn)
{
    isrectangleselection_ = yn;
    for ( int idx = editors_.size()-1; idx>=0; idx-- )
	editors_[idx]->setSelectionPolygonRectangle( isrectangleselection_ );
}


void uiFlatViewAuxDataEditorList::listSelChangeCB( CallBacker* )
{
    for ( int idx = editors_.size()-1; idx>=0; idx-- )
	editors_[idx]->setAddAuxData( -1 );
	
    if ( listbox_->nrSelected()==1 ) 
    {
	const int idx = listbox_->nextSelected();
	listboxeditors_[idx]->setAddAuxData( listboxids_[idx] );
    }
    
    change_.trigger();
}


int uiFlatViewAuxDataEditorList::findEditorIDPair( 
	const FlatView::AuxDataEditor* editor, int id ) const
{
    for ( int idx=0; idx<listboxeditors_.size(); idx++ )
    {
	if ( listboxeditors_[idx]==editor && listboxids_[idx]==id )
	    return idx;
    }
    
    return -1;
}
