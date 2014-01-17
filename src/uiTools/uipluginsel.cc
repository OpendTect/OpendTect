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
#include <math.h>

const char* uiPluginSel::sKeyDoAtStartup() { return "dTect.Select Plugins"; }

static const char* sKeydGBName = "dGB Earth Sciences";
static const char* sKeyARKCLSLTD = "ARK CLS Ltd";
static const char* sKeyARKCLSLimited = "ARK CLS Limited";
static const char* sKeyEWARKCLS = "Earthworks & ARK CLS";
static const char* sKeyARKEX = "ARKeX Ltd";
static const char* sKeySitfal = "SITFAL S.A.";

static const char* sKeyuidGBCommon = "dGB Common [dGB]";
static const char* sKeydGBPreStack = "Pre-Stack Utilities [dGB]";
static const char* sKeydGBMVA = "Multi-Variate Analysis [dGB]";
static const char* sKeyPDF3D = "3D PDF Generation";
static const char* sKeyPetrelConn = "ARK CLS Petrel Connector";
static const char* sKeyXField2D = "XField2D";
static const char* sKeyCLAS = "Computer Log Analysis Software";

static const char* sKeyStochInv = "MPSI Stochastic Inversion";
static const char* sKeyDetInv = "MPSI Deterministic Inversion";
static const char* sKey2dErrGrid = "MPSI 2D Error Grid ";
static const char* sKey3dModelBuilder = "MPSI 3D Model Builder";
static const char* sKeyMPSIUtilities = "MPSI Utilities";
static const char* sKeyMPSINettoGross = "MPSI Net to Gross";
static const char* sKeyVolAn = "MPSI Volumetrics & Connectivity";


static const char* sKeySCI = "Seismic Coloured Inversion";
static const char* sKeySFE = "Seismic Feature Enhancement";
static const char* sKeySNP = "Seismic Net Pay";
static const char* sKeySSB = "Seismic Spectral Blueing";
static const char* sKeyCCB = "CCB [dGB]";
static const char* sKeySSIS = "SSIS [dGB]";
static const char* sKeyHC = "Horizon Cube [dGB]";
static const char* sKeyVMB = "VMB [dGB]";
static const char* sKeyDS = "Dip-Steering [dGB]";
static const char* sKeyNN = "Neural Networks [dGB]";
static const char* sKeySynthRock = "SynthRock [dGB]";
static const char* sKeyWCP = "Well Correlation [dGB]";
static const char* sKeyHCSl = "Horizon Cube Slider [dGB]";
static const char* sKeyWSA = "Workstation Access link";


static bool shouldisplayinUI( const char* picreatorname, const char* dispnm )
{
    const FixedString creatorname = picreatorname;
    const FixedString displaynm = dispnm;
    const bool rejectdisplay = strstr( displaynm, "MPSI" )  ||
			       displaynm == sKeyuidGBCommon ||
			       displaynm == sKeydGBMVA      ||
			       displaynm == sKeydGBPreStack ||
			       displaynm == sKeyHCSl	    ||
			       displaynm == sKeyVolAn;

    const bool iscommercial = creatorname == sKeydGBName   || 
			      creatorname == sKeyARKCLSLTD ||
			      creatorname == sKeyARKCLSLimited ||
			      creatorname == sKeyEWARKCLS  ||
			      creatorname == sKeyARKEX	   ||
			      creatorname == sKeySitfal;

    const bool shoulddisplay = ( displaynm == sKeyStochInv ||
				 displaynm == sKeyDetInv   || !rejectdisplay );
				 

    return shoulddisplay && iscommercial;
}

const char* getCommercialName( const char* piname )
{
#define mMkUIName( nm, vennm ) \
    BufferString( nm ).add(" (").add(vennm).add(")").buf();

    static BufferString pluginname;
    pluginname = piname;
    if ( pluginname == sKeyPetrelConn )
	pluginname = mMkUIName( "Petrel Connector", "ARK CLS" );
    if ( pluginname == sKeyPDF3D )
	pluginname = mMkUIName( "PDF3D", "ARK CLS" );
    if ( pluginname == sKeyXField2D )
	pluginname = mMkUIName( "XField2D", "ARKeX" );
    if ( pluginname == sKeyCLAS )
	pluginname =
	mMkUIName( "Computer Log Analysis Software - CLAS", "SITFAL" );
    if ( pluginname == sKeyStochInv )
	pluginname = mMkUIName( "Stochastic Inversion", "Earthworks && ARK CLS");
    if ( pluginname == sKeyDetInv )
	pluginname =
	mMkUIName( "Deterministic Inversion", "Earthworks && ARK CLS" );
    if ( pluginname == sKeySCI || pluginname == sKeySFE ||
	 pluginname == sKeySNP || pluginname == sKeySSB )	
	pluginname = mMkUIName( pluginname, "ARK CLS" );
    if (  pluginname == sKeyWSA )
	pluginname = mMkUIName( "Workstation Access", "ARK CLS" );
    if ( pluginname == sKeyCCB )
	pluginname = mMkUIName( "Fluid Contact Finder - CCB", "dGB" );
    if ( pluginname == sKeySSIS )
	pluginname =
	mMkUIName("Sequence Stratigraphic Interpretation System - SSIS","dGB");
    if ( pluginname == sKeyHC )
	pluginname = mMkUIName( "HorizonCube", "dGB" );
    if ( pluginname == sKeyVMB )
	pluginname = mMkUIName( "Velocity Model Building - VMB", "dGB" );
    if ( pluginname == sKeyDS )
	pluginname = mMkUIName( "Dip Steering", "dGB" );
    if ( pluginname == sKeyNN )
	pluginname = mMkUIName( "Neural Networks", "dGB" );
    if ( pluginname == sKeySynthRock )
	pluginname = mMkUIName( "SynthRock", "dGB" );
    if ( pluginname == sKeyWCP )
	pluginname = mMkUIName( "Well Correlation Panel - WCP", "dGB" );

    return pluginname;
}

uiPluginSel::uiPluginSel( uiParent* p )
	: uiDialog(p,Setup("",mNoDlgTitle,"0.2.6")
			.savebutton(true)
			.savetext("Show this dialog at startup"))
{
    BufferString titl( "OpendTect V" );
    titl += GetFullODVersion(); titl += ": Candidate auto-loaded plugins";
    setCaption( titl );

    setCtrlStyle( uiDialog::LeaveOnly );
    setCancelText( "&Ok" );
    setSaveButtonChecked( true );

    FileMultiString dontloadlist;
    PIM().getNotLoadedByUser( dontloadlist );

    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    BufferStringSet piusrnms;
    TypeSet<int> pluginidx;
    int maxlen = 0;
    for ( int idx=0; idx<pimdata.size(); idx++ )
    {
	PluginManager::Data& data = *pimdata[idx];
	if ( data.sla_ && data.sla_->isOK()
	  && data.autotype_ == PI_AUTO_INIT_LATE )
	{
	    const FixedString dispname = data.info_->dispname;
	    if ( !shouldisplayinUI(data.info_->creator,dispname) )
		continue;
	    pluginidx += idx;
	    piusrnms.add( PIM().userName(data.name_) );
	    const int strln =
		strlen( getCommercialName(PIM().userName(data.name_)) );
	    maxlen = maxlen < strln ? strln : maxlen;
	}
    }

    const int nrplugins = piusrnms.size();
    uiLabel* lbl = new uiLabel( this, "Please select the plugins to load");
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );
    grp->attach( centeredBelow, lbl );
    CallBack cllbk( mCB(this,uiPluginSel,checkCB) );
    TypeSet<SeparString> stringset;
    pluginnms_.erase();
    BufferStringSet list;
    makeList( list );
    ObjectSet<uiCheckBox> cbs;
    for ( int idx=0; idx<nrplugins; idx++ )
    {
	BufferString uinm = getCommercialName(piusrnms.get( idx ));
	PluginManager::Data& pdata = *pimdata[pluginidx[idx]];
	SeparString ss;
	ss.add( uinm ).add( pdata.name_ );
	stringset += ss;
	uiCheckBox* cb = new uiCheckBox( grp, uinm, cllbk );
	BufferString dispnm = piusrnms.get( idx );
	cb->setChecked( dontloadlist.indexOf( dispnm )==-1 );
	cb->setPrefWidthInChar( maxlen+5.f );
	cbs += cb;
    }

    for ( int idx=0; idx<list.size(); idx++ )
    {
	const BufferString& nm = list.get(idx);
	const int lidx = getPluginIndex( nm, cbs );
	if ( lidx < 0 )
	    continue;
	cbs_ += cbs[lidx];
    }

    const char* pname = getLibName(cbs_[0]->text(),stringset);
    if ( pname )
	pluginnms_.add( pname ); 
    for ( int idx=1; idx<cbs_.size(); idx++ )
    {
	if ( idx < nrplugins/2 )
	    cbs_[idx]->attach( alignedBelow, cbs_[ idx-1] );
	else
	    cbs_[idx]->attach( rightOf, cbs_[(idx-nrplugins/2)] );
	const char* piname = getLibName(cbs_[idx]->text(),stringset);
	if ( piname )
	    pluginnms_.add( piname ); 
    }
}


bool uiPluginSel::rejectOK( CallBacker* )
{
    FileMultiString dontloadlist;
    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
	if ( !cbs_[idx]->isChecked() )
	    dontloadlist += PIM().userName( pluginnms_.get(idx) );
    }

    if ( dontloadlist.indexOf(sKeyDetInv) >= 0 )
	dontloadlist.add( sKey2dErrGrid ).add( sKey3dModelBuilder )
		    .add( sKeyMPSIUtilities ).add( sKeyMPSINettoGross );
    
    if ( dontloadlist.indexOf(sKeyStochInv) >= 0 )
	dontloadlist.add( sKeyMPSIUtilities ).add( sKeyMPSINettoGross )
		    .add( sKeyVolAn );
    
    Settings::common().setYN( sKeyDoAtStartup(), saveButtonChecked() );
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.rep() );

    Settings::common().write();

    return true;
}


int uiPluginSel::getPluginIndex( const char* pinm,
				 ObjectSet<uiCheckBox>& cbs )
{
    const FixedString uinm = pinm;
    for( int idx=0; idx<cbs.size(); idx++ )
    {
	if ( uinm == cbs[idx]->text() )
	    return idx;
    }

    return -1;
}


void uiPluginSel::setChecked( const char* pinm, bool yn )
{
    const int idx = getPluginIndex( pinm, cbs_ );
    if ( idx >= 0 )
	cbs_[idx]->setChecked( yn );
}


void uiPluginSel::checkCB( CallBacker* cb )
{
    mDynamicCastGet(uiCheckBox*,checkbox,cb);
    if ( !checkbox )
	return;

    const char* ds = "Dip Steering (dGB)";
    const char* hc = "HorizonCube (dGB)";
    const char* ssis = "Sequence Stratigraphic Interpretation System"
		       " - SSIS (dGB)";
    
    const char* si = "Stochastic Inversion (Earthworks && ARK CLS)" ;
    const char* di = "Deterministic Inversion (Earthworks && ARK CLS)";

    const FixedString dispnm = checkbox->text();
    if ( dispnm == si && checkbox->isChecked() )
	setChecked( di, true );
    else if ( dispnm == di )
    {
	if( !checkbox->isChecked() )
	    setChecked( si, false );
    }
    else if ( dispnm == ds ) 
    {
	if ( !checkbox->isChecked() )
	{
	    setChecked( hc, false );
	    setChecked( ssis, false );
	}
    }
    else if ( dispnm == hc )
    {
	if ( checkbox->isChecked() )
	    setChecked( ds, true  );
	else
	    setChecked( ssis, false );
    }
    else if ( dispnm == ssis )
    {
	if ( checkbox->isChecked() )
	{
	    setChecked( ds, true );
	    setChecked( hc, true );
	}
    }
}


const char* uiPluginSel::getLibName( const char* usrnm, 
				  const TypeSet<SeparString>& stringset ) const
{
    for ( int idx=0; idx<stringset.size(); idx++ )
    {
	const SeparString& ss = stringset[idx];
	const FixedString fs = ss[0];
	if ( fs == usrnm )
	    return ss[1];
    }

    return 0;
}


void uiPluginSel::makeList( BufferStringSet& list )
{

#define mMkName( nm, vennm ) \
    list.add( BufferString( nm ).add(" (").add(vennm).add(")").buf() );
        
    mMkName( "Dip Steering", "dGB" );
    mMkName( "HorizonCube", "dGB" );
    mMkName( "Sequence Stratigraphic Interpretation System - SSIS","dGB");
    mMkName( "Neural Networks", "dGB" );
    mMkName( "Well Correlation Panel - WCP", "dGB" );
    mMkName( "Fluid Contact Finder - CCB", "dGB" );
    mMkName( "Velocity Model Building - VMB", "dGB" );
    mMkName( "SynthRock", "dGB" );
    mMkName( sKeySCI, "ARK CLS" );
    mMkName( sKeySSB, "ARK CLS" );
    mMkName( sKeySFE, "ARK CLS" );
    mMkName( sKeySNP, "ARK CLS" );
    mMkName( "Workstation Access", "ARK CLS" );
    mMkName( "PDF3D", "ARK CLS" );
    mMkName( "Deterministic Inversion", "Earthworks && ARK CLS" );
    mMkName( "Stochastic Inversion", "Earthworks && ARK CLS");
    mMkName( "Computer Log Analysis Software - CLAS", "SITFAL" );
    mMkName( "XField2D", "ARKeX" );
    mMkName( "Petrel Connector", "ARK CLS" );
}

