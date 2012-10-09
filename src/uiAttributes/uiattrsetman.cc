/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiattrsetman.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uitextedit.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "ctxtioobj.h"
#include "survinfo.h"

mDefineInstanceCreatedNotifierAccess(uiAttrSetMan)


uiAttrSetMan::uiAttrSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Attribute Set file management",
				     "Manage attribute sets",
				     "101.3.0").nrstatusflds(1),
	           AttribDescSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiAttrSetMan::~uiAttrSetMan()
{
}


static void addAttrNms( const Attrib::DescSet& attrset, BufferString& txt,
			bool stor )
{
    const int totnrdescs = attrset.nrDescs( true, true );
    int nrdisp = 0;
    for ( int idx=0; idx<totnrdescs; idx++ )
    {
	const Attrib::Desc* desc = attrset.desc( idx );
	if ( desc->isHidden()
	  || (stor && !desc->isStored()) || (!stor && desc->isStored()) )
	    continue;

	if ( nrdisp > 0 )
	    txt += ", ";
	txt += desc->userRef();

	nrdisp++;
	if ( nrdisp > 2 )
	    { txt += ", ..."; break; }
    }
}


void uiAttrSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    Attrib::DescSet attrset( !SI().has3D() );
    if ( !AttribDescSetTranslator::retrieve(attrset,curioobj_,txt) )
    {
	BufferString msg( "Read error: '" ); msg += txt; msg += "'";
	txt = msg;
    }
    else
    {
	if ( !txt.isEmpty() )
	    ErrMsg( txt );

	const int nrattrs = attrset.nrDescs( false, false );
	const int nrwithstor = attrset.nrDescs( true, false );
	const int nrstor = nrwithstor - nrattrs;
	txt = "Type: "; txt += attrset.is2D() ? "2D" : "3D";
	if ( nrstor > 0 )
	{
	    txt += "\nInput"; txt += nrstor == 1 ? ": " : "s: "; 
	    addAttrNms( attrset, txt, true );
	}
	if ( nrattrs < 1 )
	    txt += "\nNo attributes defined";
	else
	{
	    txt += "\nAttribute"; txt += nrattrs == 1 ? ": " : "s: "; 
	    addAttrNms( attrset, txt, false );
	}
    }

    txt += "\n";
    txt += getFileInfo();
    setInfo( txt );
}
