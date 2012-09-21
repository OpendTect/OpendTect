/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uigeninputdlg.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
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


NotifierAccess* uiGenInputGrp::enterClose()
{
    if ( flds.size()==1 && flds[0]->nrElements()==1 )
    {
	mDynamicCastGet( uiLineEdit*, ule, flds[0]->element( 0 ) );
	if ( ule )
	    return &ule->returnPressed;
    }

    return 0;
}

const char* uiGenInputGrp::text( int idx )
{ return flds[idx]->text( 0 ); }
int uiGenInputGrp::getIntValue( int idx )
{ return flds[idx]->getIntValue( 0 ); }
float uiGenInputGrp::getfValue( int idx )
{ return flds[idx]->getfValue( 0 ); }
double uiGenInputGrp::getdValue( int idx )
{ return flds[idx]->getdValue( 0 ); }
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
    }

    return true;
}



uiGenInputDlg::uiGenInputDlg( uiParent* p, const char* dlgtitle,
			      const char* fldtxt, DataInpSpec* spec )
	: uiDialog(p,Setup("Input data",dlgtitle,mNoHelpID))
{
    group = new uiGenInputGrp( this, dlgtitle, fldtxt, spec );
    postFinalise().notify( mCB( this, uiGenInputDlg, setEnterClose ) );
}


uiGenInputDlg::uiGenInputDlg( uiParent* p, const char* dlgtitle,
			      ObjectSet<uiGenInputDlgEntry>* e )
	: uiDialog(p,Setup("Input data",dlgtitle,mNoHelpID))
{
    group = new uiGenInputGrp( this, dlgtitle, e );
    postFinalise().notify( mCB( this, uiGenInputDlg, setEnterClose ) );
}


void uiGenInputDlg::setEnterClose(CallBacker*)
{
    if ( group->enterClose() )
    {
	group->enterClose()->notify( mCB( this, uiDialog, accept ));
	if ( group->nrFlds() )
	    group->getFld(0)->setFocus();
    }
}


const char* uiGenInputDlg::text( int idx )
{ return group->text(idx); }
int uiGenInputDlg::getIntValue( int idx )
{ return group->getIntValue( idx ); }
float uiGenInputDlg::getfValue( int idx )
{ return group->getfValue( idx ); }
double uiGenInputDlg::getdValue( int idx )
{ return group->getdValue( idx ); }
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
