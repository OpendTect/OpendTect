/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uizaxistransform.h"

#include "datainpspec.h"
#include "refcount.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "zaxistransform.h"

mImplFactory3Param( uiZAxisTransform, uiParent*, const char*, const char*,
		    uiZAxisTransform::factory );


uiZAxisTransform::uiZAxisTransform( uiParent* p ) 
    : uiDlgGroup( p, 0 )
{}


uiZAxisTransformSel::uiZAxisTransformSel( uiParent* p, bool withnone,
	const char* fromdomain, const char* todomain )
    : uiGroup( p )
{
    transflds_.allowNull( true );
    transdlgs_.allowNull( true );
    TypeSet<const char*> names;

    const BufferStringSet& factorynames =
	uiZAxisTransform::factory().getNames();

    const BufferStringSet& usernames =
	uiZAxisTransform::factory().getNames(true);

    uiDialog::Setup settings( 0, 0, mNoHelpID );
    uiDialog* dlg = new uiDialog( this, settings );
    for ( int idx=0; idx<factorynames.size(); idx++ )
    {
	uiZAxisTransform* uizat = uiZAxisTransform::factory().create(
		factorynames[idx]->buf(), dlg, fromdomain, todomain );
	if ( !uizat )
	    continue;

	transflds_ += uizat;
	transdlgs_ += dlg;
	names += usernames[idx]->buf();

	if ( idx==factorynames.size()-1 )
	    dlg = 0;
	else
	    dlg = new uiDialog( this, settings );
    }

    delete dlg;

    const char* nonestr = "None";

    if ( !names.size() )
    {
	transflds_ += 0;
	transdlgs_ += 0;
	names +=  nonestr;
    }
    else if ( withnone )
    {
	transflds_.insertAt( 0, 0 );
	transdlgs_.insertAt( 0, 0 );
	names.insert( 0, nonestr );
    }

    names += 0;
    selfld_ = new uiGenInput( this, "Z transform",
	    StringListInpSpec(names.arr()) );
    selfld_->valuechanged.notify( mCB(this, uiZAxisTransformSel,selCB) );

    settingsbut_ = new uiPushButton( this, "Settings", 
	    mCB( this, uiZAxisTransformSel, settingsCB), false );
    settingsbut_->attach( rightOf, selfld_ );

    setHAlignObj( selfld_ );

    selCB( 0 );
}


NotifierAccess* uiZAxisTransformSel::selectionDone()
{
    return &selfld_->valuechanged;
}


int uiZAxisTransformSel::nrTransforms() const
{
    int res = transflds_.size();
    if ( res && !transflds_[0] )
	res--;
    return res;
}


void uiZAxisTransformSel::settingsCB( CallBacker* )
{
    const int idx = selfld_->getIntValue();
    if ( !transdlgs_[idx] )
	return;

    transdlgs_[idx]->go();
}


ZAxisTransform* uiZAxisTransformSel::getSelection()
{
    const int idx = selfld_->getIntValue();
    return transflds_[idx] ? transflds_[idx]->getSelection() : 0;
}


bool uiZAxisTransformSel::fillPar( IOPar& par )
{
    RefMan<ZAxisTransform> sel = getSelection();
    if ( !sel )
	return false;

    sel->fillPar( par );
    return true;
}


FixedString uiZAxisTransformSel::getZDomain() const
{
    const int idx = selfld_->getIntValue();
    if ( !transflds_[idx] )
	return FixedString( 0 );

    return transflds_[idx]->getZDomain();
}


bool uiZAxisTransformSel::acceptOK()
{
    const int idx = selfld_->getIntValue();
    if ( !transflds_[idx] )
	return true;

    return transflds_[idx]->acceptOK();
}


void uiZAxisTransformSel::selCB( CallBacker* )
{
    const int selidx = selfld_->getIntValue();
    settingsbut_->display( transdlgs_[selidx] );
}
