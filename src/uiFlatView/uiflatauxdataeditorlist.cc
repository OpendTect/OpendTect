/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditorlist.cc,v 1.2 2007-05-10 13:12:51 cvskris Exp $
________________________________________________________________________

-*/

#include "uiflatauxdataeditorlist.h"

#include "flatauxdataeditor.h"
#include "uibutton.h"
#include "uilistbox.h"


uiFlatViewAuxDataEditorList::uiFlatViewAuxDataEditorList(uiParent* p,
					FlatView::AuxDataEditor* editor )
    : uiGroup( p )
    , editor_( editor )
{
    listbox_ = new uiListBox( this );
    listbox_->setMultiSelect( true );
    listbox_->selectionChanged.notify(
	    mCB(this,uiFlatViewAuxDataEditorList,listSelChangeCB) );

    addbutton_ = new uiPushButton( this, "Add", false );
    addbutton_->attach( alignedBelow, listbox_ );

    removebutton_ = new uiPushButton( this, "Remove", true );
    removebutton_->attach( rightOf, addbutton_ );

    updateList( 0 );
}


uiFlatViewAuxDataEditorList::~uiFlatViewAuxDataEditorList()
{ }


void uiFlatViewAuxDataEditorList::setEditor( FlatView::AuxDataEditor* newed )
{
    editor_ = newed;
    updateList( 0 );
}


void uiFlatViewAuxDataEditorList::updateList( CallBacker* )
{
    const TypeSet<int>* ids = editor_ ? &editor_->getIds() : 0;
    const ObjectSet<FlatView::Annotation::AuxData>* auxdata =
	editor_ ? &editor_->getAuxData() : 0;

    listbox_->empty();
    ids_.erase();

    if ( ids ) 
    {
	for ( int idx=0; idx<ids->size(); idx++ )
	{
	    const FlatView::Annotation::AuxData* ad = (*auxdata)[idx];
	    if ( !ad->markerstyle_.color.isVisible() || !ad->enabled_ )
		continue;

	    listbox_->insertItem( ad->name_, ad->markerstyle_.color,
		    		  listbox_->size() );
	    ids_ += (*ids)[idx];
	}
    }

    const int selidx =
	ids_.size() ? ids_.indexOf( editor_->getAddAuxData() ) : -1;
    
    NotifyStopper block( listbox_->selectionChanged );
    if ( selidx<0 )
	listbox_->selectAll( false );
    else
	listbox_->setSelected( selidx, true );

    removebutton_->setSensitive( ids_.size() );
}


void uiFlatViewAuxDataEditorList::setSelection( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 ) return;

    listbox_->selectAll( false );
    listbox_->setSelected( idx, true );
}


NotifierAccess* uiFlatViewAuxDataEditorList::addNotifier()
{ return &addbutton_->activated; }


NotifierAccess* uiFlatViewAuxDataEditorList::removeNotifier()
{ return &removebutton_->activated; }


void uiFlatViewAuxDataEditorList::enableAdd( bool yn )
{
    addbutton_->setSensitive( yn );
}


void uiFlatViewAuxDataEditorList::listSelChangeCB( CallBacker* )
{
    const int nrsel = listbox_->nrSelected();
    if ( nrsel!=1 ) editor_->setAddAuxData( -1 );
    else editor_->setAddAuxData( ids_[listbox_->nextSelected()] );
}
