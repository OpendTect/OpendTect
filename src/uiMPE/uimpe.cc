/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimpe.cc,v 1.7 2009-07-22 16:01:40 cvsbert Exp $";

#include "uimpe.h"

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



uiSetupGroup::uiSetupGroup( uiParent* p, const char* ref )
    : uiGroup( p, "Tracking Setup" )
    , helpref_(ref)
{}


bool uiSetupGroup::commitToTracker() const
{
    bool dummybool;
    return commitToTracker( dummybool );
}


void uiSetupGroupFactory::addFactory( uiSetupGrpCreationFunc f )
{ funcs += f; }


uiSetupGroup* uiSetupGroupFactory::create( uiParent* p, const char* typestr,
					   const Attrib::DescSet* ads )
{
    for ( int idx=0; idx<funcs.size(); idx++ )
    {
	uiSetupGroup* res = funcs[idx](p,typestr,ads);
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
	
    }

    return *uiengine;
}

};
