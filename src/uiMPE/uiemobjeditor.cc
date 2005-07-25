/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: uiemobjeditor.cc,v 1.1 2005-07-25 12:22:36 cvskris Exp $
________________________________________________________________________

-*/

#include "uiemobjeditor.h"

#include "emeditor.h"

namespace MPE
{

uiEMObjectEditor::uiEMObjectEditor( uiParent* p, MPE::ObjectEditor* he )
    : uiEMEditor( p )
    , editor( he )
    , snapmenuitem( "Snap after edit" )
{}


void uiEMObjectEditor::createNodeMenus(CallBacker* cb)
{
    if ( node.objectID()==-1 ) return;
    mDynamicCastGet( MenuHandler*, menu, cb );

    mAddMenuItem( menu, &snapmenuitem, editor->canSnapAfterEdit(node), 
	   	  editor->getSnapAfterEdit() );
}


void uiEMObjectEditor::handleNodeMenus(CallBacker* cb)
{
    
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==snapmenuitem.id )
    {
	menu->setIsHandled(true);
	editor->setSnapAfterEdit(!editor->getSnapAfterEdit());
    }
}


};
