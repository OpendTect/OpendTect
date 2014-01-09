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
static const char* sKeySCI = "Seismic Coloured Inversion";
static const char* sKeySFE = "Seismic Feature Enhancement";
static const char* sKeySNP = "Seismic Net Pay";
static const char* sKeySSB = "Seismic Spectral Blueing";
static const char* sKeyVolAn = "MPSI Volumetrics & Connectivity";
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
			       displaynm == sKeyHCSl;

    const bool iscommercial = creatorname == sKeydGBName   || 
			      creatorname == sKeyARKCLSLTD ||
			      creatorname == sKeyARKCLSLimited ||
			      creatorname == sKeyEWARKCLS  ||
			      creatorname == sKeyARKEX	   ||
			      creatorname == sKeySitfal;

    const bool shoulddisplay = ( displaynm == sKeyStochInv ||
				 displaynm == sKeyDetInv   ||
				 displaynm == sKeyVolAn    || !rejectdisplay );

    return shoulddisplay && iscommercial;
}

const char* getCommercialName( const char* piname )
{
    BufferString bs;
#define mMkUIName( nm, vennm ) \
    bs.add( nm ).add(" (").add(vennm).add(")").buf();

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
	pluginname = mMkUIName( "Stochastic Inversion", "Earthworks & ARK CLS");
    if ( pluginname == sKeyDetInv )
	pluginname =
	mMkUIName( "Deterministic Inversion", "Earthworks & ARK CLS" );
    if ( pluginname == sKeyVolAn )
	pluginname = mMkUIName( "Volumetric Analysis", "Earthworks & ARK CLS" );
    if ( pluginname == sKeySCI || pluginname == sKeySFE ||
	 pluginname == sKeySNP || pluginname == sKeySSB )	
	pluginname = mMkUIName( pluginname, "ARK CLS" );
    if (  pluginname == sKeyWSA )
	pluginname = mMkUIName( "Workstation Access", "ARK CLS" );
    if ( pluginname == sKeyCCB )
	pluginname = mMkUIName( "Common Contour Binning - CCB", "dGB" );
    if ( pluginname == sKeySSIS )
	pluginname =
	mMkUIName("Sequence Stratigraphic Interpretation System - SSIS","dGB");
    if ( pluginname == sKeyHC )
	pluginname = mMkUIName( "HorizonCube - HC", "dGB" );
    if ( pluginname == sKeyVMB )
	pluginname = mMkUIName( "Velocity Model Building - VMB", "dGB" );
    if ( pluginname == sKeyDS )
	pluginname = mMkUIName( "Dip Steering - DS", "dGB" );
    if ( pluginname == sKeyNN )
	pluginname = mMkUIName( "Neural Networks - NN", "dGB" );
    if ( pluginname == sKeySynthRock )
	pluginname = mMkUIName( "SynthRock - SR", "dGB" );
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

    ArrPtrMan<int> sortindices = piusrnms.getSortIndexes();

    const float rowspercol = maxlen / 10.f;
    const int nrplugins = piusrnms.size();
    int nrcols = 2;
    if ( nrcols < 1 ) nrcols = 1;
    if ( nrcols > 3 ) nrcols = 3;
    int nrows = nrplugins / nrcols;
    if ( nrplugins % nrcols )
	nrows++;

    uiLabel* lbl = new uiLabel( this, "Please select the plugins to load");
    uiGroup* grp = new uiGroup( this, "OpendTect plugins to load" );
    grp->setFrame( true );
    grp->attach( centeredBelow, lbl );

    for ( int idx=0; idx<nrplugins; idx++ )
    {
	const int realidx = sortindices[idx];
	const int colnr = idx / nrows;
	const int rownr = idx - colnr * nrows;

	BufferString uinm = getCommercialName(piusrnms.get( realidx ));
	uiCheckBox* cb = new uiCheckBox( grp, uinm );

	PluginManager::Data& pdata = *pimdata[pluginidx[realidx]];
	pluginnms_.add( pdata.name_ );
	BufferString dispnm = piusrnms.get( realidx );
	cb->setChecked( dontloadlist.indexOf( dispnm )==-1 );
	cbs_ += cb;
	if ( colnr != nrcols - 1 )
	    cb->setPrefWidthInChar( maxlen+5.f );
	if ( idx == 0 ) continue;

	if ( rownr == 0 )
	    cb->attach( rightOf, cbs_[(colnr-1)*nrows] );
	else
	    cb->attach( alignedBelow, cbs_[idx-1] );
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

    Settings::common().setYN( sKeyDoAtStartup(), saveButtonChecked() );
    Settings::common().set( PluginManager::sKeyDontLoad(), dontloadlist.rep() );

    Settings::common().write();

    return true;
}
