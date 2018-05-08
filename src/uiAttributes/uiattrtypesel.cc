/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
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

uiString uiAttrTypeSel::sAllGroup()
{
    return uiStrings::sAll().embedFinalState();
}


uiAttrTypeSel::uiAttrTypeSel( uiParent* p, bool sorted )
    : uiGroup(p,"Attribute type selector")
    , sorted_(sorted)
    , sortidxs_(0)
    , selChg(this)
{
    groupfld_ = new uiComboBox( this, "Attribute group" );
    groupfld_->selectionChanged.notify( mCB(this,uiAttrTypeSel,groupSel) );
    attrfld_ = new uiComboBox( this, "Attribute type" );
    attrfld_->selectionChanged.notify( mCB(this,uiAttrTypeSel,attrSel) );
    attrfld_->attach( rightOf, groupfld_ );
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
    groupnms_.setEmpty(); attrnms_.setEmpty();
    groupidxs_.erase();
    delete [] sortidxs_; sortidxs_ = 0;

    groupnms_.add( sAllGroup() );
}


void uiAttrTypeSel::setEmpty()
{
    clear();
    groupfld_->setEmpty();
    attrfld_->setEmpty();
}


void uiAttrTypeSel::fill( uiStringSet* selgroups )
{
    setEmpty();
    const int forbiddendomtyp = (int)(SI().zIsTime() ? uiAttrDescEd::Depth
						     : uiAttrDescEd::Time);

    for ( int iattr=0; iattr<uiAF().size(); iattr++ )
    {
	const uiString groupnm = uiAF().getGroupName( iattr );
	if ( uiAF().domainType(iattr) != forbiddendomtyp
	  && (!selgroups || selgroups->isPresent(groupnm)) )
	    add( groupnm, uiAF().getDisplayName(iattr) );
    }

    update();
}


uiString uiAttrTypeSel::groupName() const
{
    const int attridx = attrnms_.indexOf( attributeDisplayName() );
    return groupnms_.get( groupidxs_[attridx] );
}


uiString uiAttrTypeSel::attributeDisplayName() const
{
    return attrfld_->uiText();
}


const char* uiAttrTypeSel::attributeName() const
{
    return uiAF().attrNameOf( attributeDisplayName() );
}


void uiAttrTypeSel::setGroupName( const uiString& group )
{
    if ( !groupnms_.isPresent(group) )
	return;
    groupfld_->setText( group );
    updAttrNms();
}


void uiAttrTypeSel::setAttributeName( const char* attrnm )
{
    if ( attrnm && *attrnm )
	setAttributeDisplayName( uiAF().dispNameOf(attrnm) );
}


void uiAttrTypeSel::setAttributeDisplayName( const uiString& attrnm )
{
    const int attridx = attrnms_.indexOf( attrnm );
    if ( attridx < 0 )
	return;

    int groupidx = groupidxs_[attridx];
    int oldgroupitem = groupfld_->currentItem();
    int oldgroupidx = sortidxs_ ? sortidxs_[oldgroupitem] : oldgroupitem;
    if ( oldgroupitem == 0 || groupidx == oldgroupidx )
	attrfld_->setText( attrnm );
    else
    {
	groupfld_->setText( groupnms_.get(groupidx) );
	updAttrNms( &attrnm );
    }
}


void uiAttrTypeSel::add( const uiString& gnm, const uiString& attrnm )
{
    if ( attrnm.isEmpty() )
	return;

    const uiString groupnm( gnm.isEmpty() ? sAllGroup() : gnm );
    int groupidx = groupnms_.indexOf( groupnm );
    if ( groupidx < 0 )
    {
	groupnms_.add( groupnm );
	groupidx = groupnms_.size() - 1;
    }

    attrnms_.add( attrnm );
    groupidxs_ += groupidx;
}


void uiAttrTypeSel::update()
{
    groupfld_->setEmpty(); attrfld_->setEmpty();
    const int nrgroups = groupnms_.size();

    delete [] sortidxs_;
    sortidxs_ = sorted_ ? groupnms_.getSortIndexes(true,true) : 0;

    for ( int idx=0; idx<nrgroups; idx++ )
	groupfld_->addItem( groupnms_.get(sortidxs_ ? sortidxs_[idx] : idx) );

    setGroupName( uiAttrDescEd::sBasicGrp() );
}


int uiAttrTypeSel::curGroupIdx() const
{
    if ( groupnms_.size() < 1 )
	return -1;

    const int groupitem = groupfld_->currentItem();
    return sortidxs_ ? sortidxs_[groupitem] : groupitem;
}


void uiAttrTypeSel::updAttrNms( const uiString* selattrnm )
{
    uiString curattrnm;
    const bool haveselattrnm = selattrnm && !selattrnm->isEmpty();
    if ( haveselattrnm )
	curattrnm = *selattrnm;
    else
	curattrnm = attrfld_->uiText();

    attrfld_->setEmpty();
    const int groupidx = curGroupIdx();
    if ( groupidx < 0 )
	return;

    uiStringSet nms;
    for ( int idx=0; idx<attrnms_.size(); idx++ )
    {
	if ( groupidx == 0 || groupidxs_[idx] == groupidx )
	    nms.add( attrnms_.get(idx) );
    }

    nms.sort( true, true );
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiString attrnm = nms.get( idx );
	attrfld_->addItem( attrnm );
	if ( !haveselattrnm && isGroupDef(attrnm) )
	    curattrnm = attrnm;
    }

    if ( !curattrnm.isEmpty() )
	attrfld_->setText( curattrnm );
}


void uiAttrTypeSel::groupSel( CallBacker* )
{
    updAttrNms();
    selChg.trigger();
}


void uiAttrTypeSel::attrSel( CallBacker* )
{
    selChg.trigger();
}


bool uiAttrTypeSel::isGroupDef( const uiString& anm ) const
{
    uiString grp4query = anm;
    if ( grp4query == sAllGroup() )
	grp4query = uiAttrDescEd::sBasicGrp();
    return uiAF().isGroupDef( uiAF().indexOf(grp4query) );
}
