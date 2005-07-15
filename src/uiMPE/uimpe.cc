/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: uimpe.cc,v 1.1 2005-07-15 13:54:57 cvskris Exp $
________________________________________________________________________

-*/

#include "uimpe.h"

#include "uiemhorizoneditor.h"

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


uiMPEEngine& uiMPE()
{
    static PtrMan<uiMPEEngine> uiengine = 0;
    if ( !uiengine )
    {
	uiengine = new uiMPEEngine;
	
	//init standard classes
	uiEMHorizonEditor::initClass();
    }

    return *uiengine;
}

};
