/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: uimpe.cc,v 1.2 2005-12-14 18:52:52 cvskris Exp $
________________________________________________________________________

-*/

#include "uimpe.h"

#include "uiemhorizoneditor.h"
#include "uifaulttracksetup.h"
#include "uihorizontracksetup.h"

namespace MPE
{


uiEMEditor::uiEMEditor(uiParent* p )
    : parent ( p )
    , node( -1, -1, -1 )
{}


uiEMEditor::~uiEMEditor() {}
   


void uiEMEditorFactory::addFactory( uiEMEditorCreationFunc f ) { funcs += f; }


uiEMEditor* uiEMEditorFactory::create( uiParent* p, MPE::ObjectEditor* e ) const
{
    for ( int idx=0; idx<funcs.size(); idx++ )
    {
	uiEMEditor* res = funcs[idx](p,e);
	if ( res ) return res;
    }

    return 0;
}



uiSetupDialog::uiSetupDialog( uiParent* p, const char* helpref )
    : uiDialog( p, uiDialog::Setup("Tracking Setup",0,helpref).modal(false) )
{}


void uiSetupDialogFactory::addFactory( uiSetupDlgCreationFunc f )
{ funcs += f; }


uiSetupDialog* uiSetupDialogFactory::create( uiParent* p,  SectionTracker* st,
					     const Attrib::DescSet* ds )
{
    for ( int idx=0; idx<funcs.size(); idx++ )
    {
	uiSetupDialog* res = funcs[idx](p,st,ds);
	if ( res ) return res;
    }

    return 0;
}


uiMPEEngine& uiMPE()
{
    static PtrMan<uiMPEEngine> uiengine = 0;
    if ( !uiengine )
    {
	uiengine = new uiMPEEngine;
	
	//init standard classes
	uiEMHorizonEditor::initClass();
	uiHorizonSetupDialog::initClass();
	uiFaultSetupDialog::initClass();
    }

    return *uiengine;
}

};
