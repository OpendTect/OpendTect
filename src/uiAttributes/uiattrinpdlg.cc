/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattrinpdlg.h"

#include "bufstringset.h"
#include "uisteeringsel.h"
#include "uitextedit.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "ctxtioobj.h"
#include "hiddenparam.h"
#include "ioman.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "od_helpids.h"

HiddenParam<uiAttrInpDlg,ObjectSet<uiSeisSel>* > seisinpsetmgr( 0 );
HiddenParam<uiAttrInpDlg,ObjectSet<uiSeisSel>* > steerinpsetmgr( 0 );

uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset, 
			    bool issteer, bool is2d, const char* prevrefnm )
    : uiDialog(p,uiDialog::Setup(tr("Attribute set definition"),
		       issteer ? tr("Select Steering input")
			       : tr("Select Seismic input"),
				 mODHelpKey(mAttrInpDlgHelpID) ))
    , multiinpcube_(true)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
{
    seisinpsetmgr.setParam(this, new ObjectSet<uiSeisSel>);
    steerinpsetmgr.setParam(this, new ObjectSet<uiSeisSel>);
    uiString infotxt = tr( "Provide input for the following attributes: " );
    uiLabel* infolbl = new uiLabel( this, infotxt );

    BufferString txt;
    for ( int idx=0; idx<refset.size(); idx++ )
	{ txt += refset.get(idx); txt += "\n"; }
    
    uiTextEdit* txtfld = new uiTextEdit( this, "File Info", true );
    txtfld->setPrefHeightInChar( 10 );
    txtfld->setPrefWidthInChar( 40 );
    txtfld->setText( txt );
    txtfld->attach( alignedBelow, infolbl );

    uiString seltext = issteer ? uiStrings::phrInput(tr("SteeringCube")) 
			       : uiStrings::phrInput(uiStrings::sSeismics());
    if ( prevrefnm )
    {
	seltext = tr("%1\n (replacing '%2')").arg(seltext)
					     .arg(toUiString(prevrefnm));
    }

    if ( issteer )
    {
	*steerinpsetmgr.getParam(this) +=
		new uiSteerCubeSel( this, is2d, true, seltext );
	(*steerinpsetmgr.getParam(this))[0]->attach( alignedBelow, txtfld );
    }
    else
    {
	uiSeisSel::Setup sssu( is2d, false );
	sssu.seltxt( seltext );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	*seisinpsetmgr.getParam(this) += new uiSeisSel( this, ioctxt, sssu );
	(*seisinpsetmgr.getParam(this))[0]->attach( alignedBelow, txtfld );
    }

}


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, bool hasseis, bool hassteer,
       			    bool is2d )
    : uiDialog(p,uiDialog::Setup(tr("Attribute set definition"),
	       hassteer ? (hasseis ? tr("Select Seismic & Steering input")
			: tr("Select Steering input")) 
                        : tr("Select Seismic input"),
			  mODHelpKey(mAttrInpDlgHelpID) ))
    , multiinpcube_(false)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
{
    seisinpsetmgr.setParam(this, new ObjectSet<uiSeisSel>);
    steerinpsetmgr.setParam(this, new ObjectSet<uiSeisSel>);
    if ( hasseis )
    {
	uiSeisSel::Setup sssu( is2d, false );
	sssu.seltxt( uiStrings::phrInput(uiStrings::sSeismics() ) );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	*seisinpsetmgr.getParam(this) += new uiSeisSel( this, ioctxt, sssu );
    }

    if ( hassteer )
    {
	*steerinpsetmgr.getParam(this) +=
	    new uiSteerCubeSel( this, is2d, true,
				uiStrings::phrInput( uiStrings::sSteering() ) );
    }

    const int seisinpsz = seisinpsetmgr.getParam(this)->size();

    if ( steerinpsetmgr.getParam(this)->size()
	&& (*steerinpsetmgr.getParam(this))[0] && seisinpsz )
	(*steerinpsetmgr.getParam(this))[0]->attach( alignedBelow,
				(*seisinpsetmgr.getParam(this))[seisinpsz-1] );
}


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& seisinpnms,
			    const BufferStringSet& steeringinpnms, bool is2d )
    : uiDialog(p,uiDialog::Setup(tr("Attribute set definition"),
		 steeringinpnms.size()
		    ? (seisinpnms.size() ? tr("Select Seismic & Steering input")
					 : tr("Select Steering input"))
		    : tr("Select Seismic input"),
		 mODHelpKey(mAttrInpDlgHelpID) ))
    , multiinpcube_(false)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
{
    seisinpsetmgr.setParam(this, new ObjectSet<uiSeisSel>);
    steerinpsetmgr.setParam(this, new ObjectSet<uiSeisSel>);
    for ( int idx=0; idx<seisinpnms.size(); idx++ )
    {
	uiSeisSel::Setup sssu( is2d, false );
	if ( seisinpnms.size()>1 )
	{
	    BufferString displaydatanm = seisinpnms.get(idx);
	    displaydatanm.embed('"','"');
	    sssu.seltxt( uiStrings::phrInput(toUiString("%1 %2")
			   .arg(uiStrings::sSeismics())
			   .arg(displaydatanm ) ) );
	}
	else
	    sssu.seltxt( uiStrings::phrInput(
				uiStrings::sVolDataName(is2d, !is2d, false)) );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	*seisinpsetmgr.getParam(this) += new uiSeisSel( this, ioctxt, sssu );
    }

    for ( int idx=0; idx<steeringinpnms.size(); idx++ )
    {
	uiString tmpstr;
	if ( steeringinpnms.size()>1 )
	{
	    BufferString steerdatanm = steeringinpnms.get(idx);
	    steerdatanm.remove("_inline_dip");
	    steerdatanm.remove("_crline_dip");
	    steerdatanm.embed('"','"');
	    tmpstr = uiStrings::phrInput(toUiString("%1 %2")
					   .arg(uiStrings::sSteering())
					   .arg(steerdatanm) );
	}
	else
	    tmpstr = uiStrings::phrInput( uiStrings::sSteering() );

	*steerinpsetmgr.getParam(this) +=
				new uiSteerCubeSel( this, is2d, true, tmpstr );
    }

    const int seisinpsz = seisinpsetmgr.getParam(this)->size();
    for ( int idx=1; idx<seisinpsz; idx++ )
	(*seisinpsetmgr.getParam(this))[idx]->attach( alignedBelow,
				(*seisinpsetmgr.getParam(this))[idx-1] );

    if ( steerinpsetmgr.getParam(this)->size()
	&& (*steerinpsetmgr.getParam(this))[0] && seisinpsz )
	(*steerinpsetmgr.getParam(this))[0]->attach( alignedBelow,
				(*seisinpsetmgr.getParam(this))[seisinpsz-1] );

    for ( int idx=1; idx<steerinpsetmgr.getParam(this)->size(); idx++ )
	(*steerinpsetmgr.getParam(this))[idx]->attach( alignedBelow,
				(*steerinpsetmgr.getParam(this))[idx-1] );
}


#define mErrRetSelInp() \
    { \
	uiMSG().error( errmsg ); \
	return false; \
    }


bool uiAttrInpDlg::acceptOK( CallBacker* )
{
    const uiString errmsg = uiStrings::phrSelect(tr("the input for the "
								"attributes"));

    for ( int idx=0; idx<steerinpsetmgr.getParam(this)->size(); idx++ )
	if ( (*steerinpsetmgr.getParam(this))[idx]
		&& !(*steerinpsetmgr.getParam(this))[idx]->commitInput() )
	    mErrRetSelInp();

    for ( int idx=0; idx<seisinpsetmgr.getParam(this)->size(); idx++ )
	if ( (*seisinpsetmgr.getParam(this))[idx]
		&& !(*seisinpsetmgr.getParam(this))[idx]->commitInput() )
	    mErrRetSelInp();

    return true;
}


uiAttrInpDlg::~uiAttrInpDlg()
{
    delete seisinpsetmgr.getParam(this);
    seisinpsetmgr.removeParam( this );
    delete steerinpsetmgr.getParam(this);
    steerinpsetmgr.removeParam( this );
}


const char* uiAttrInpDlg::getSeisRef() const
{
    return getSeisRefFromIndex(0);
}


const char* uiAttrInpDlg::getSteerRef() const
{
    return getSteerRefFromIndex(0);
}


const char* uiAttrInpDlg::getSeisKey() const
{
    return getSeisKeyFromIndex(0);
}


const char* uiAttrInpDlg::getSteerKey() const
{
    return getSeisKeyFromIndex(0);
}


const char* uiAttrInpDlg::getSeisRefFromIndex( int idx ) const
{
    return seisinpsetmgr.getParam(this)->size()>idx
	&& (*seisinpsetmgr.getParam(this))[idx]
		? (*seisinpsetmgr.getParam(this))[idx]->getInput() : 0;
}


const char* uiAttrInpDlg::getSteerRefFromIndex( int idx ) const
{
    return steerinpsetmgr.getParam(this)->size()>idx
	&& (*steerinpsetmgr.getParam(this))[idx]
		? (*steerinpsetmgr.getParam(this))[idx]->getInput() : 0;
}


const char* uiAttrInpDlg::getSeisKeyFromIndex( int idx ) const
{
    if ( seisinpsetmgr.getParam(this)->size()<=idx
	|| !(*seisinpsetmgr.getParam(this))[idx] )
	return 0;

    LineKey lk;
    const IOObj* ioobj = (*seisinpsetmgr.getParam(this))[idx]->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}


const char* uiAttrInpDlg::getSteerKeyFromIndex( int idx ) const
{
    if ( steerinpsetmgr.getParam(this)->size()<=idx
	|| !(*steerinpsetmgr.getParam(this))[idx] )
	return 0;

    static LineKey lk;
    const IOObj* ioobj = (*steerinpsetmgr.getParam(this))[idx]->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}
