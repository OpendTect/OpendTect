/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: uiemhorizoneditor.cc,v 1.2 2005-07-21 20:59:05 cvskris Exp $
________________________________________________________________________

-*/

#include "uiemhorizoneditor.h"

#include "datainpspec.h"
#include "emhorizoneditor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimpepartserv.h"
#include "uimsg.h"

namespace MPE
{


class uiEMHorizonEditorSetting : public uiDialog
{
public:
    		uiEMHorizonEditorSetting( uiParent*, HorizonEditor* );

protected:
    bool	acceptOK(CallBacker*);

    uiGenInput*		horshapefld;
    uiGenInput*		horsizefld;
    uiGenInput*		vertshapefld;
    HorizonEditor*	editor;
};


uiEMHorizonEditorSetting::uiEMHorizonEditorSetting( uiParent* p,
						    HorizonEditor* he )
    : uiDialog( p, uiDialog::Setup("Horizon Editor","Settings") )
    , editor( he )
{
    horshapefld = new uiGenInput( this, "Horizontal shape",
	    			  BoolInpSpec("Box", "Ellipse",
				  editor->boxEditArea() ) );

    horsizefld = new uiGenInput( this, "Horizontal size",
	    BinIDCoordInpSpec( false, true, false,
			       editor->getEditArea().row,
			       editor->getEditArea().col) );
    horsizefld->attach( alignedBelow, horshapefld );

    if ( editor->getVertMovingStyleNames() )
    {
	StringListInpSpec spec( *editor->getVertMovingStyleNames() );
	spec.setValue( editor->getVertMovingStyle() );
	vertshapefld =  new uiGenInput( this, "Vertical shape", spec );
	vertshapefld->attach( alignedBelow, horsizefld );
    }
    else
	vertshapefld = 0;
}


bool uiEMHorizonEditorSetting::acceptOK(CallBacker*)
{
    const RowCol rc( horsizefld->getIntValue(0), horsizefld->getIntValue(1) );
    const Interval<int> range( 0,25 );
    if ( !range.includes(rc.row) || !range.includes(rc.col) )
    {
	BufferString msg = "Allowed size is 0 to ";
	msg += range.stop;
	msg += ".";
	uiMSG().error(msg);
	return false;
    }

    editor->setEditArea(rc);
    if ( vertshapefld ) editor->setVertMovingStyle(vertshapefld->getIntValue());
    editor->setBoxEditArea(horshapefld->getBoolValue());

    return true;
}


void uiEMHorizonEditor::initClass()
{
    uiMPE().editorfact.addFactory( uiEMHorizonEditor::create );
}


uiEMEditor* uiEMHorizonEditor::create( uiParent* p, MPE::ObjectEditor* e )
{
    mDynamicCastGet( MPE::HorizonEditor*, he, e );
    return he ? new uiEMHorizonEditor( p, he ) : 0;
}


uiEMHorizonEditor::uiEMHorizonEditor( uiParent* p, MPE::HorizonEditor* he )
    : uiEMObjectEditor( p, he )
{}


void uiEMHorizonEditor::createNodeMenus(CallBacker* cb)
{
    if ( node.objectID()==-1 ) return;
    mDynamicCastGet( MenuHandler*, menu, cb );

    uiEMObjectEditor::createNodeMenus(cb);

    mAddMenuItem( menu, &editsettingsmnuitem, true, false );
}


void uiEMHorizonEditor::handleNodeMenus(CallBacker* cb)
{
    uiEMObjectEditor::handleNodeMenus(cb);
    
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==editsettingsmnuitem.id )
    {
	menu->setIsHandled(true);

	uiEMHorizonEditorSetting dlg( parent,
		reinterpret_cast<HorizonEditor*>( editor) );
	dlg.go();
    }
}


};
