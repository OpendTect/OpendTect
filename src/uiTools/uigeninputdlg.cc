/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uigeninputdlg.cc,v 1.5 2004-01-28 14:55:28 kristofer Exp $
________________________________________________________________________

-*/

#include "uigeninputdlg.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"


uiGenInputGrp::uiGenInputGrp( uiParent* p, const char* dlgtitle,
			      const char* fldtxt, DataInpSpec* spec )
	: uiGroup(p,dlgtitle)
	, entries(new ObjectSet<uiGenInputDlgEntry>)
{
    *entries += new uiGenInputDlgEntry( fldtxt, spec );
    build();
}


uiGenInputGrp::uiGenInputGrp( uiParent* p, const char* dlgtitle,
			      ObjectSet<uiGenInputDlgEntry>* e )
	: uiGroup(p,dlgtitle)
	, entries(e?e : new ObjectSet<uiGenInputDlgEntry>)
{
    build();
}


void uiGenInputGrp::build()
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


const char* uiGenInputGrp::text( int idx )
{ return flds[idx]->text( 0 ); }
int uiGenInputGrp::getIntValue( int idx )
{ return flds[idx]->getIntValue( 0 ); }
float uiGenInputGrp::getfValue( int idx )
{ return flds[idx]->getfValue( 0 ); }
double uiGenInputGrp::getValue( int idx )
{ return flds[idx]->getValue( 0 ); }
bool uiGenInputGrp::getBoolValue( int idx )
{ return flds[idx]->getBoolValue( 0 ); }


bool uiGenInputGrp::acceptOK( CallBacker* )
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



uiGenInputDlg::uiGenInputDlg( uiParent* p, const char* dlgtitle,
			      const char* fldtxt, DataInpSpec* spec )
	: uiDialog(p,Setup("Input data",dlgtitle))
{
    group = new uiGenInputGrp( this, dlgtitle, fldtxt, spec );
}


uiGenInputDlg::uiGenInputDlg( uiParent* p, const char* dlgtitle,
			      ObjectSet<uiGenInputDlgEntry>* e )
	: uiDialog(p,Setup("Input data",dlgtitle))
{
    group = new uiGenInputGrp( this, dlgtitle, e );
}


const char* uiGenInputDlg::text( int idx )
{ return group->text(idx); }
int uiGenInputDlg::getIntValue( int idx )
{ return group->getIntValue( idx ); }
float uiGenInputDlg::getfValue( int idx )
{ return group->getfValue( idx ); }
double uiGenInputDlg::getValue( int idx )
{ return group->getValue( idx ); }
bool uiGenInputDlg::getBoolValue( int idx )
{ return group->getBoolValue( idx ); }


int uiGenInputDlg::nrFlds() const
{ return group->nrFlds(); }
uiGenInput* uiGenInputDlg::getFld(int idx)
{ return group->getFld(idx); }


bool uiGenInputDlg::acceptOK( CallBacker* cb )
{
    return group->acceptOK( cb );
}
