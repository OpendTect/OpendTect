/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uigeninputdlg.cc,v 1.1 2002-05-28 08:41:18 bert Exp $
________________________________________________________________________

-*/

#include "uigeninputdlg.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"


uiGenInputDlg::uiGenInputDlg( uiParent* p, const char* dlgtitle,
			      const char* fldtxt, DataInpSpec* spec )
	: uiDialog(p,Setup("Input data",dlgtitle))
	, entries(new ObjectSet<uiGenInputDlgEntry>)
{
    *entries += new uiGenInputDlgEntry( fldtxt, spec );
    build();
}


uiGenInputDlg::uiGenInputDlg( uiParent* p, const char* dlgtitle,
			      ObjectSet<uiGenInputDlgEntry>* e )
	: uiDialog(p,Setup("Input data",dlgtitle))
	, entries(e?e : new ObjectSet<uiGenInputDlgEntry>)
{
    build();
}


void uiGenInputDlg::build()
{
    if ( !entries->size() )
    {
	new uiLabel( this, "There are no values to be set" );
	return;
    }

    for ( int idx=0; idx<entries->size(); idx++ )
    {
	uiGenInputDlgEntry& entry = *((*entries)[idx]);
	if ( !entry.spec ) entry.spec = new StringInpSpec;
	uiGenInput* fld = new uiGenInput( this, entry.txt, *entry.spec );
	flds += fld;

	if ( idx )
	    fld->attach( alignedBelow, flds[idx-1] );
    }
}


const char* uiGenInputDlg::text( int idx )
{ return flds[idx]->text( 0 ); }
int uiGenInputDlg::getIntValue( int idx )
{ return flds[idx]->getIntValue( 0 ); }
float uiGenInputDlg::getfValue( int idx )
{ return flds[idx]->getfValue( 0 ); }
double uiGenInputDlg::getValue( int idx )
{ return flds[idx]->getValue( 0 ); }
bool uiGenInputDlg::getBoolValue( int idx )
{ return flds[idx]->getBoolValue( 0 ); }


bool uiGenInputDlg::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<entries->size(); idx++ )
    {
	uiGenInputDlgEntry& entry = *((*entries)[idx]);
	if ( !entry.allowundef && flds[idx]->isUndef() )
	{
	    BufferString msg = "Please enter a value for '";
	    msg += entry.txt; msg += "'";
	    uiMSG().error( msg );
	    return false;
	}
	if ( !entry.allowinvalid && !flds[idx]->isValid() )
	{
	    BufferString msg = "Please enter a valid input for '";
	    msg += entry.txt; msg += "'";
	    uiMSG().error( msg );
	    return false;
	}
    }

    return true;
}
