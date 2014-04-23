/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipluginsel.h"
#include "uibutton.h"
#include "uigroup.h"
#include "uilabel.h"
#include "plugins.h"
#include "settings.h"
#include "separstr.h"
#include "odver.h"
#include "od_helpids.h"

#include <math.h>

const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.Select Plugins"; }

struct PluginProduct
{
    BufferString	    productname_;
    BufferStringSet	    libs_;
};


uiPluginSel::uiPluginSel( uiParent* p )
	: uiDialog(p,Setup("",mNoDlgTitle,
                            mODHelpKey(mPluginSelHelpID) )
			.savebutton(true)
			.savetext(tr("Show this dialog at startup")))
	, maxpluginname_(0)
{
    BufferString titl( "OpendTect V" );
    titl += GetFullODVersion(); titl +=
			  tr(": Candidate auto-loaded plugins").getFullString();
    setCaption( tr(titl) );

    setOkText( tr("Start OpendTect") );
    setSaveButtonChecked( true );
        
    const ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    makeProductList( pimdata );
    
    createUI();
}


uiPluginSel::~uiPluginSel()
{
    deepErase( products_ );
}


void uiPluginSel::makeProductList(
				const ObjectSet<PluginManager::Data>& pimdata )
{
    PluginProduct* product = 0;
    for ( int idx=0; idx<pimdata.size(); idx++ )
    {
	const PluginManager::Data& data = *pimdata[idx];
	if ( data.sla_ && data.sla_->isOK()
			&& data.autotype_ == PI_AUTO_INIT_LATE )
	{
	    const FixedString prodnm = data.info_->productname_;
	    if ( data.info_->lictype_ != PluginInfo::COMMERCIAL 
				      || prodnm == "OpendTect (dGB)" )
		continue;
	    
	    const int prodidx = getProductIndex( data.info_->productname_ );
	    if ( !product || prodidx < 0 )
	    {
		product = new PluginProduct();
		product->productname_ = data.info_->productname_;
		product->libs_.add( PIM().userName(data.name_) );
		products_ += product;
	    }
	    else
		products_[prodidx]->libs_.addIfNew( PIM().userName(data.name_));

	    const int strln = strlen( product->productname_ );
	    maxpluginname_ = maxpluginname_ < strln ? strln : maxpluginname_;
	}
    }
}


void uiPluginSel::createUI()
{
    uiLabel* lbl = new uiLabel( this, tr("Please select the plugins to load"));
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );
    grp->attach( centeredBelow, lbl );

    const int nrproducts = products_.size();
    const int nrrows = nrproducts % 2 == 0 ? (nrproducts/2) : (nrproducts/2)+1;
    for ( int idx=0; idx<nrproducts; idx++ )
    {
	uiCheckBox* cb = new uiCheckBox( grp, products_[idx]->productname_ );
	cb->setPrefWidthInChar( maxpluginname_+5.f );
	cb->setChecked( true );
	cbs_ += cb;

	if ( idx < nrrows )
	{
	    if ( idx == 0 )
		continue;
	    cb->attach( alignedBelow, cbs_[idx-1] );
	}
	else
	    cb->attach( rightOf, cbs_[(idx-nrrows)] );
    }

    BufferStringSet dontloadlist;
    PIM().getNotLoadedByUser( dontloadlist );
    for ( int idx=0; idx<dontloadlist.size() && !cbs_.isEmpty(); idx++ )
    {
	const int prodidx = getProductIndexForLib( dontloadlist.get(idx) );
	if( prodidx < 0 )
	    continue;
	cbs_[prodidx]->setChecked( false );
    }
}


bool uiPluginSel::acceptOK( CallBacker* )
{
    FileMultiString dontloadlist;
    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
	if ( !cbs_[idx]->isChecked() )
	{
	    for( int idp=0; idp<products_[idx]->libs_.size(); idp++ )
		dontloadlist += products_[idx]->libs_.get(idp);
	}
    }

    Settings::common().setYN( sKeyDoAtStartup(), saveButtonChecked() );
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.rep() );
    Settings::common().write();

    return true;
}


int uiPluginSel::getProductIndex( const char* prodnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->productname_ == prodnm )
	    return idx;
    }

    return -1;
}


int uiPluginSel::getProductIndexForLib( const char* libnm ) const
{
    for ( int idx=0; idx<products_.size(); idx++ )
    {
	if ( products_[idx]->libs_.isPresent(libnm) )
	    return idx;
    }

    return -1;
}
