/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uiattrinpdlg.h"

#include "bufstringset.h"
#include "uisteeringsel.h"
#include "uitextedit.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "od_helpids.h"


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset, 
			    bool issteer, bool is2d, const char* prevrefnm )
    : uiDialog(p,uiDialog::Setup(tr("Attribute set definition"),
		       issteer ? tr("Select Steering input")
			       : tr("Select Seismic input"),
				 mODHelpKey(mAttrInpDlgHelpID) ))
    , multiinpcube_(true)
    , is2d_(is2d)
{
    seisinpflds_.erase();
    steerinpflds_.erase();
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
			       : uiStrings::phrInput(uiStrings::sSeismicData());
    if ( prevrefnm )
    {
	seltext = tr("%1\n (replacing '%2')").arg(seltext)
					     .arg(toUiString(prevrefnm));
    }

    if ( issteer )
    {
	steerinpflds_ += new uiSteerCubeSel( this, is2d, true, seltext );
	steerinpflds_[0]->attach( alignedBelow, txtfld );
    }
    else
    {
	uiSeisSel::Setup sssu( is2d, false );
	sssu.seltxt( seltext );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	seisinpflds_ += new uiSeisSel( this, ioctxt, sssu );
	seisinpflds_[0]->attach( alignedBelow, txtfld );
    }

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
{
    for ( int idx=0; idx<seisinpnms.size(); idx++ )
    {
	uiSeisSel::Setup sssu( is2d, false );
	if ( seisinpnms.size()>1 )
	{
	    BufferString displaydatanm = seisinpnms.get(idx);
	    displaydatanm.embed('"','"');
	    sssu.seltxt( uiStrings::phrInput(toUiString("%1 %2")
			   .arg(uiStrings::sSeismicData())
			   .arg(displaydatanm ) ) );
	}
	else
	    sssu.seltxt( uiStrings::phrInput(
				uiStrings::sVolDataName(is2d, !is2d, false)) );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	seisinpflds_ += new uiSeisSel( this, ioctxt, sssu );
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

	steerinpflds_ += new uiSteerCubeSel( this, is2d, true, tmpstr );
    }

    const int seisinpsz = seisinpflds_.size();
    for ( int idx=1; idx<seisinpsz; idx++ )
	seisinpflds_[idx]->attach( alignedBelow,seisinpflds_[idx-1] );

    if ( steerinpflds_.size() && steerinpflds_[0] && seisinpsz )
	steerinpflds_[0]->attach( alignedBelow, seisinpflds_[seisinpsz-1] );

    for ( int idx=1; idx<steerinpflds_.size(); idx++ )
	steerinpflds_[idx]->attach( alignedBelow, steerinpflds_[idx-1] );
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

    for ( int idx=0; idx<steerinpflds_.size(); idx++ )
	if ( steerinpflds_[idx] && !steerinpflds_[idx]->commitInput() )
	    mErrRetSelInp();

    for ( int idx=0; idx<seisinpflds_.size(); idx++ )
	if ( seisinpflds_[idx] && !seisinpflds_[idx]->commitInput() )
	    mErrRetSelInp();

    return true;
}


uiAttrInpDlg::~uiAttrInpDlg()
{
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
    return seisinpflds_.size()>idx && seisinpflds_[idx]
		? seisinpflds_[idx]->getInput() : 0;
}


const char* uiAttrInpDlg::getSteerRefFromIndex( int idx ) const
{
    return steerinpflds_.size()>idx && steerinpflds_[idx]
		? steerinpflds_[idx]->getInput() : 0;
}


const char* uiAttrInpDlg::getSeisKeyFromIndex( int idx ) const
{
    if ( seisinpflds_.size()<=idx || !seisinpflds_[idx] )
	return 0;

    LineKey lk;
    const IOObj* ioobj = seisinpflds_[idx]->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}


const char* uiAttrInpDlg::getSteerKeyFromIndex( int idx ) const
{
    if ( steerinpflds_.size()<=idx || !steerinpflds_[idx] )
	return 0;

    static LineKey lk;
    const IOObj* ioobj = steerinpflds_[idx]->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}
