/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2018
________________________________________________________________________

-*/

#include "uiiconsetsel.h"
#include "settings.h"
#include "dirlist.h"
#include "oddirs.h"
#include "uitoolbar.h"
#include "uicombobox.h"

const char* uiIconSetSel::sKeyIconSetNm() { return "Icon set name"; }
const char* uiIconSetSel::sKeyDefaultIconSetNm() { return "Default"; }


static BufferString curSetName()
{
    BufferString ret;
    Settings::common().get( uiIconSetSel::sKeyIconSetNm(), ret );
    if ( ret.isEmpty() )
	ret.set( uiIconSetSel::sKeyDefaultIconSetNm() );
    return ret;
}


static void addSetNames( const DirList& dl, BufferStringSet& nms )
{
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString nm( dl.get(idx).buf() + 6 ); // 6 for "icons."
	if ( !nm.isEmpty() && !nms.isPresent(nm) )
	    nms.add( nm );
    }
}


static void doActivateSet( const char* nm )
{
    Settings::common().set( uiIconSetSel::sKeyIconSetNm(), nm );
    for ( int idx=0; idx<uiToolBar::toolBars().size(); idx++ )
	uiToolBar::toolBars()[idx]->reloadIcons();
}


uiIconSetSel::uiIconSetSel( uiParent* p, const BufferStringSet& setnms,
			  bool withlabel )
    : uiGroup(p,"Icon properties")
    , iconsetnms_(setnms)
    , setnameatentry_(curSetName())
{
    if ( withlabel )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
					uiStrings::sIcon(mPlural) );
	selfld_ = lcb->box();
	setHAlignObj( lcb );
    }
    else
    {
	selfld_ = new uiComboBox( this, "Icons" );
	setHAlignObj( selfld_ );
    }

    for ( int idx=0; idx<iconsetnms_.size(); idx++ )
    {
	const BufferString& nm = iconsetnms_.get( idx );
	uiString txt; BufferString icnm;
	if ( nm == sKeyDefaultIconSetNm() )
	{
	    txt = tr("Default Icons");
	    icnm = "od";
	}
	else
	{
	    const BufferString coolnm( "'", nm, "'" );
	    txt = toUiString( coolnm );
	    icnm = nm; icnm.toLower();
	}
	selfld_->addItem( txt );
	selfld_->setIcon( idx, icnm );
    }

    int cursetidx = iconsetnms_.indexOf( setnameatentry_ );
    if ( cursetidx < 0 )
    {
	setnameatentry_.set( sKeyDefaultIconSetNm() );
	cursetidx = iconsetnms_.indexOf( setnameatentry_ );
    }
    selfld_->setCurrentItem( cursetidx<0 ? 0 : cursetidx );

    mAttachCB( selfld_->selectionChanged, uiIconSetSel::setSel );
    selfld_->setToolTip( tr("Select an icon set") );
}


void uiIconSetSel::getSetNames( BufferStringSet& nms )
{
    const char* msk = "icons.*";
    const DirList dlsett( GetSettingsDir(), File::DirsInDir, msk );
    const DirList dlsite( mGetApplSetupDataDir(), File::DirsInDir, msk );
    const DirList dlrel( mGetSWDirDataDir(), File::DirsInDir, msk );

    nms.add( sKeyDefaultIconSetNm() );

    addSetNames( dlsett, nms );
    addSetNames( dlsite, nms );
    addSetNames( dlrel, nms );
}



void uiIconSetSel::setSel( CallBacker* )
{
    const int selidx = selfld_->currentItem();
    if ( selidx >= 0 )
	activateSet( iconsetnms_.get(selidx) );
}


bool uiIconSetSel::newSetSelected() const
{
    return setnameatentry_ != curSetName();
}


void uiIconSetSel::revert()
{
    if ( newSetSelected() )
	activateSet( setnameatentry_, true );
}


void uiIconSetSel::activateSet( const char* nm, bool force )
{
    const BufferString cursetnm = curSetName();
    if ( force || cursetnm != nm )
	doActivateSet( nm );
}


bool uiIconSetSel::setODIconSet( const char* nm, bool mkpermanent )
{
    const BufferString oldsetnm = curSetName();
    doActivateSet( nm );
    if ( mkpermanent )
	Settings::common().write();
    return oldsetnm != curSetName();
}
