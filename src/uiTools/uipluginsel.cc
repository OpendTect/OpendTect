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
#include "uipixmap.h"

#include "plugins.h"
#include "settings.h"
#include "separstr.h"
#include "odver.h"
#include "od_helpids.h"
#include "odlogo24x24.xpm"

#include <math.h>

const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.Select Plugins"; }
static const char* getCreatorShortName( const BufferString& creatornm )
{
    return creatornm == "dGB Earth Sciences" ? "(dGB)"
			: creatornm == "SITFAL" ? "(SITFAL)" : "";
}


struct PluginProduct
{
    BufferString	    productname_;
    BufferString	    creator_;
    BufferStringSet	    libs_;
    int			    posval_;
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
    makeGlobalProductList();
    const ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    makeProductList( pimdata );
    createUI();
}


uiPluginSel::~uiPluginSel()
{
    deepErase( products_ );
}


void uiPluginSel::makeGlobalProductList()
{

#define mAddProduct(nm,crnm,posval) \
    product = new PluginProduct(); \
		product->productname_ = nm; \
		product->creator_ = crnm; \
		product->posval_ = posval; \
		products_ += product; \

    PluginProduct* product = 0;
    mAddProduct( "Dip Steering", "dGB Earth Sciences", 1 );
    mAddProduct( "HorizonCube", "dGB Earth Sciences", 2 );
    mAddProduct( "Sequence Stratigraphic Interpretation System - SSIS",
				   "dGB Earth Sciences", 3 );
    mAddProduct( "Neural Networks", "dGB Earth Sciences", 4 );
    mAddProduct( "Well Correlation Panel - WCP", "dGB Earth Sciences", 5 );
    mAddProduct( "Fluid Contact Finder", "dGB Earth Sciences", 6 );
    mAddProduct( "Velocity Model Building - VMB", "dGB Earth Sciences", 7 );
    mAddProduct( "SynthRock", "dGB Earth Sciences", 8 );
    mAddProduct( "Seismic Coloured Inversion (ARK CLS)", "ARK CLS", 9 );
    mAddProduct( "Seismic Spectral Blueing (ARK CLS)", "ARK CLS", 10);
    mAddProduct( "Seismic Feature Enhancement (ARK CLS)", "ARK CLS", 11 );
    mAddProduct( "Seismic Net Pay (ARK CLS)", "ARK CLS", 12);
    mAddProduct( "MPSI (Earthworks & ARK CLS)", "Earthworks & ARK CLS", 13 );
    mAddProduct( "Multi-Volume Seismic Enhancement (ARK CLS)", "ARK CLS", 14 );
    mAddProduct( "Workstation Access (ARK CLS)", "ARK CLS", 15 );
    mAddProduct( "CLAS Computer Log Analysis Software", "SITFAL", 16 );
    mAddProduct( "XField (ARKeX)", "ARKeX", 17 );
}


void uiPluginSel::makeProductList(
				const ObjectSet<PluginManager::Data>& pimdata )
{
    for ( int idx=0; idx<pimdata.size(); idx++ )
    {
	const PluginManager::Data& data = *pimdata[idx];
	if ( data.sla_ && data.sla_->isOK() )
	{
	    const FixedString prodnm = data.info_->productname_;
	    const bool isodprod = prodnm.isEmpty() || prodnm == "OpendTect";
	    if ( data.info_->lictype_ != PluginInfo::COMMERCIAL || isodprod )
		continue;

	    const int prodidx = getProductIndex( data.info_->productname_ );
	    if ( prodidx<0 )
	    {
		PluginProduct* product = new PluginProduct();
		product->productname_ = data.info_->productname_;
		product->creator_ = data.info_->creator_;
		product->libs_.add( PIM().moduleName(data.name_) );
		products_ += product;
	    }
	    else
		products_[prodidx]->libs_.addIfNew(
				PIM().moduleName(data.name_) );

	    const int strsz = prodnm.size() + 6;
	    maxpluginname_ = maxpluginname_ < strsz ? strsz : maxpluginname_;
	}
    }

    for ( int idx=products_.size()-1; idx>=0; idx-- )
    {
	const PluginProduct* product = products_[idx];
	if ( product->libs_.isEmpty() )
	    products_.removeSingle( idx );
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
	const PluginProduct& pprod = *products_[idx];
	const BufferString label( pprod.productname_, " ",
					getCreatorShortName(pprod.creator_) );
	uiCheckBox* cb = new uiCheckBox( grp, label );
	if ( !pprod.creator_.isEmpty() )
	    cb->setToolTip( BufferString("a ",pprod.creator_, " plugin") );
	cb->setPrefWidthInChar( maxpluginname_+6.f );
	cb->setChecked( true );
	cb->setStretch( 2, 0 ); 
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
