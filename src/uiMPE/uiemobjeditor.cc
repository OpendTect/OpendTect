/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
