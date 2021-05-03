/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

#include "uiattrtypesel.h"

#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uicombobox.h"

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
    grpfld_ = new uiComboBox( this, "Attribute group" );
    grpfld_->selectionChanged.notify( mCB(this,uiAttrTypeSel,grpSel) );
    attrfld_ = new uiComboBox( this, "Attribute type" );
    attrfld_->selectionChanged.notify( mCB(this,uiAttrTypeSel,attrSel) );
    attrfld_->attach( rightOf, grpfld_ );
    attrfld_->setHSzPol( uiObject::Wide );

    setHAlignObj( attrfld_ );
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


void uiAttrTypeSel::setEmpty()
{
    clear();
    grpfld_->setEmpty();
    attrfld_->setEmpty();
}


void uiAttrTypeSel::fill( BufferStringSet* selgrps )
{
    setEmpty();
    const int forbiddendomtyp = (int)(SI().zIsTime() ? uiAttrDescEd::Depth
						     : uiAttrDescEd::Time);

    for ( int iattr=0; iattr<uiAF().size(); iattr++ )
    {
	const char* grpnm = uiAF().getGroupName( iattr );
	if ( uiAF().domainType(iattr) != forbiddendomtyp
	  && (!selgrps || selgrps->isPresent(grpnm)) )
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
    return attrfld_->text();
}


void uiAttrTypeSel::setGrp( const char* grp )
{
    if ( !grpnms_.isPresent(grp) ) return;
    grpfld_->setText( grp );
    updAttrNms();
}


void uiAttrTypeSel::setAttr( const char* attrnm )
{
    const int attridx = attrnms_.indexOf( attrnm );
    if ( attridx < 0 )
	return;

    int grpidx = attrgroups_[attridx];
    int oldgrpitem = grpfld_->currentItem();
    int oldgrpidx = idxs_ ? idxs_[oldgrpitem] : oldgrpitem;
    if ( oldgrpitem == 0 || grpidx == oldgrpidx )
	attrfld_->setText( attrnm );
    else
    {
	grpfld_->setText( grpnms_.get(grpidx) );
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
    grpfld_->setEmpty(); attrfld_->setEmpty();
    const int nrgrps = grpnms_.size();

    delete [] idxs_;
    idxs_ = sorted_ ? grpnms_.getSortIndexes() : 0;

    for ( int idx=0; idx<nrgrps; idx++ )
	grpfld_->addItem( toUiString(grpnms_.get(idxs_ ? idxs_[idx] : idx)) );

    setGrp( sKeyAllGrp );
}


int uiAttrTypeSel::curGrpIdx() const
{
    if ( grpnms_.size() < 1 )
	return -1;

    const int grpitem = grpfld_->currentItem();
    return idxs_ ? idxs_[grpitem] : grpitem;
}


void uiAttrTypeSel::updAttrNms( const char* selattrnm )
{
    BufferString curattrnm( selattrnm );
    if ( !selattrnm || !*selattrnm )
	curattrnm = attrfld_->text();

    attrfld_->setEmpty();
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
    {
	const char* attrnm = nms.get(idx);
	attrfld_->addItem( toUiString(attrnm) );
	if ( (!selattrnm || !*selattrnm) && isPrefAttrib(grpidx,attrnm) )
	    curattrnm = attrnm;
    }

    if ( curattrnm )
	attrfld_->setText( curattrnm );
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


bool uiAttrTypeSel::isPrefAttrib( int grpidx, const char* anm ) const
{
    const char* grp = grpnms_.get( grpidx );

    const FixedString grpnm( grp );
    const FixedString attrnm( anm );
    if ( *grpnm == '<' )
	return attrnm == "Similarity";
    else if ( grpnm == "Basic" )
	return attrnm == "Scaling";
    else if ( grpnm == "Filters" )
	return attrnm == "Frequency Filter";
    else if ( grpnm == "Frequency" )
	return attrnm == "Spectral Decomp";
    else if ( grpnm == "Patterns" )
	return attrnm == "FingerPrint";
    else if ( grpnm == "Positions" )
	return attrnm == "Position";
    else if ( grpnm == "Statistics" )
	return attrnm == "Volume Statistics";
    else if ( grpnm == "Trace match" )
	return attrnm == "Match delta";

    return false;
}
