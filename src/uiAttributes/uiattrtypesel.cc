/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattrtypesel.cc,v 1.5 2009-07-22 16:01:37 cvsbert Exp $";

#include "uiattrtypesel.h"
#include "uiattrdesced.h"
#include "uicombobox.h"
#include "uiattribfactory.h"
#include "attribdesc.h"
#include "survinfo.h"

using namespace Attrib;

const char* uiAttrTypeSel::sKeyAllGrp = "<All>";


uiAttrTypeSel::uiAttrTypeSel( uiParent* p, bool sorted )
    : uiGroup(p,"Attribute type selector")
    , sorted_(sorted)
    , idxs_(0)
    , selChg(this)
{
    grpfld = new uiComboBox( this, "Attribute group" );
    grpfld->selectionChanged.notify( mCB(this,uiAttrTypeSel,grpSel) );
    attrfld = new uiComboBox( this, "Attribute type" );
    attrfld->selectionChanged.notify( mCB(this,uiAttrTypeSel,attrSel) );
    attrfld->attach( rightOf, grpfld );

    setHAlignObj( attrfld );
    clear();
}


uiAttrTypeSel::~uiAttrTypeSel()
{
    clear();
}


void uiAttrTypeSel::clear()
{
    grpnms_.erase(); grpnms_.add( sKeyAllGrp );
    attrnms_.erase();
    attrgroups_.erase();
    delete [] idxs_; idxs_ = 0;
}


void uiAttrTypeSel::empty()
{
    clear();
    grpfld->empty();
    attrfld->empty();
}


void uiAttrTypeSel::fill( BufferStringSet* selgrps )
{
    empty();
    const int forbiddendomtyp = (int)(SI().zIsTime() ? uiAttrDescEd::Depth
	    					     : uiAttrDescEd::Time);

    for ( int iattr=0; iattr<uiAF().size(); iattr++ )
    {
	const char* grpnm = uiAF().getGroupName( iattr );
	if ( uiAF().domainType(iattr) != forbiddendomtyp
	  && (!selgrps || selgrps->indexOf(grpnm) >= 0) )
	    add( grpnm, uiAF().getDisplayName(iattr) );
    }

    update();
}


const char* uiAttrTypeSel::group() const
{
    const int attridx = attrnms_.indexOf( attr() );
    return grpnms_.get( attrgroups_[attridx] );
}


const char* uiAttrTypeSel::attr() const
{
    return attrfld->text();
}


void uiAttrTypeSel::setGrp( const char* grp )
{
    if ( grpnms_.indexOf(grp) < 0 )
	return;

    grpfld->setText( grp );
    updAttrNms();
}


void uiAttrTypeSel::setAttr( const char* attrnm )
{
    const int attridx = attrnms_.indexOf( attrnm );
    if ( attridx < 0 )
	return;

    int grpidx = attrgroups_[attridx];
    int oldgrpitem = grpfld->currentItem();
    int oldgrpidx = idxs_ ? idxs_[oldgrpitem] : oldgrpitem;
    if ( oldgrpitem == 0 || grpidx == oldgrpidx )
	attrfld->setText( attrnm );
    else
    {
	grpfld->setText( grpnms_.get(grpidx) );
	updAttrNms( attrnm );
    }
}


void uiAttrTypeSel::add( const char* grpnm, const char* attrnm )
{
    if ( !attrnm || !*attrnm )	return;
    if ( !grpnm || !*grpnm )	grpnm = sKeyAllGrp;

    int grpidx = grpnms_.indexOf( grpnm );
    if ( grpidx < 0 )
    {
	grpnms_.add( grpnm );
	grpidx = grpnms_.size() - 1;
    }

    attrnms_.add( attrnm );
    attrgroups_ += grpidx;
}


void uiAttrTypeSel::update()
{
    grpfld->empty(); attrfld->empty();
    const int nrgrps = grpnms_.size();

    delete [] idxs_;
    idxs_ = sorted_ ? grpnms_.getSortIndexes() : 0;

    for ( int idx=0; idx<nrgrps; idx++ )
	grpfld->addItem( grpnms_.get(idxs_ ? idxs_[idx] : idx) );

    setGrp( sKeyAllGrp );
}


int uiAttrTypeSel::curGrpIdx() const
{
    if ( grpnms_.size() < 1 )
	return -1;

    const int grpitem = grpfld->currentItem();
    return idxs_ ? idxs_[grpitem] : grpitem;
}


void uiAttrTypeSel::updAttrNms( const char* selattrnm )
{
    attrfld->empty();
    const int grpidx = curGrpIdx();
    if ( grpidx < 0 )
	return;

    BufferStringSet nms;
    for ( int idx=0; idx<attrnms_.size(); idx++ )
    {
	if ( grpidx == 0 || attrgroups_[idx] == grpidx )
	    nms.add( attrnms_.get(idx) );
    }
    nms.sort();
    for ( int idx=0; idx<nms.size(); idx++ )
	attrfld->addItem( nms.get(idx) );

    if ( selattrnm )
	attrfld->setText( selattrnm );
}


void uiAttrTypeSel::grpSel( CallBacker* )
{
    updAttrNms();
    selChg.trigger();
}


void uiAttrTypeSel::attrSel( CallBacker* )
{
    selChg.trigger();
}
