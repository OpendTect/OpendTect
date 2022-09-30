/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigeninputdlg.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uistrings.h"

uiGenInputDlgEntry::uiGenInputDlgEntry( const uiString& t, DataInpSpec* s )
    : txt(t), spec(s?s:new StringInpSpec)
    , allowundef(false)
{}


uiGenInputDlgEntry::~uiGenInputDlgEntry()
{
    delete spec;
}


uiGenInputGrp::uiGenInputGrp( uiParent* p, const char* grpname,
			      uiString fldtxt, DataInpSpec* spec )
	: uiGroup(p,grpname)
	, entries(new ObjectSet<uiGenInputDlgEntry>)
{
    *entries += new uiGenInputDlgEntry( fldtxt, spec );
    build();
}


uiGenInputGrp::uiGenInputGrp( uiParent* p, const char* grpname,
			      ObjectSet<uiGenInputDlgEntry>* e )
	: uiGroup(p,grpname)
	, entries(e?e : new ObjectSet<uiGenInputDlgEntry>)
{
    build();
}


uiGenInputGrp::~uiGenInputGrp()
{ deepErase(*entries); }


void uiGenInputGrp::build()
{
    if ( !entries->size() )
    {
	new uiLabel( this, tr("There are no values to be set") );
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
float uiGenInputGrp::getFValue( int idx )
{ return flds[idx]->getFValue( 0 ); }
double uiGenInputGrp::getDValue( int idx )
{ return flds[idx]->getDValue( 0 ); }
bool uiGenInputGrp::getBoolValue( int idx )
{ return flds[idx]->getBoolValue( 0 ); }


bool uiGenInputGrp::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<entries->size(); idx++ )
    {
	uiGenInputDlgEntry& entry = *((*entries)[idx]);
	if ( !entry.allowundef && flds[idx]->isUndef() )
	{
	    uiString msg = tr("Please enter a value for '%1'")
			 .arg(entry.txt);
	    uiMSG().error( msg );
	    return false;
	}
    }

    return true;
}



uiGenInputDlg::uiGenInputDlg( uiParent* p, const uiString&  dlgtitle,
			      uiString fldtxt, DataInpSpec* spec )
	: uiDialog(p,Setup(uiStrings::sInputData(),dlgtitle,mNoHelpKey))
{
    group = new uiGenInputGrp( this, dlgtitle.getFullString(), fldtxt, spec );
    postFinalize().notify( mCB( this, uiGenInputDlg, setEnterClose ) );
}


uiGenInputDlg::uiGenInputDlg( uiParent* p, const uiString&  dlgtitle,
			      ObjectSet<uiGenInputDlgEntry>* e )
	: uiDialog(p,Setup(uiStrings::sInputData(),dlgtitle,mNoHelpKey))
{
    group = new uiGenInputGrp( this, dlgtitle.getFullString(), e );
    postFinalize().notify( mCB( this, uiGenInputDlg, setEnterClose ) );
}


uiGenInputDlg::~uiGenInputDlg()
{}


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
float uiGenInputDlg::getFValue( int idx )
{ return group->getFValue( idx ); }
double uiGenInputDlg::getDValue( int idx )
{ return group->getDValue( idx ); }
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
