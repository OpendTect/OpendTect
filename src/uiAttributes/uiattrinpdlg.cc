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
#include "ioobjctxt.h"
#include "uilabel.h"
#include "uimsg.h"
#include "oddirs.h"
#include "od_helpids.h"


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset,
			    bool issteer, bool is2d, const char* prevrefnm )
    : uiDialog(p,uiDialog::Setup(tr("Attribute set definition"),
		       issteer ? tr("Select Steering input")
			       : tr("Select Seismic input"),
				 mODHelpKey(mAttrInpDlgHelpID) ))
    , is2d_(is2d)
{
    seisinpflds_.erase();
    steerinpflds_.erase();
    uiString infotxt = tr("Provide input for the following attributes: ");
    uiLabel* infolbl = new uiLabel( this, infotxt );

    BufferString txt;
    for ( int idx=0; idx<refset.size(); idx++ )
	{ txt += refset.get(idx); txt += "\n"; }

    uiTextEdit* txtfld = new uiTextEdit( this, "File Info", true );
    txtfld->setPrefHeightInChar( 10 );
    txtfld->setPrefWidthInChar( 40 );
    txtfld->setText( txt );
    txtfld->attach( alignedBelow, infolbl );

    uiString seltext = issteer
        ? uiStrings::phrInput(uiStrings::sSteeringCube())
        : uiStrings::phrInput(uiStrings::sSeisObjName(true, true, false) );

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
    , is2d_(is2d)
{
    for ( int idx=0; idx<seisinpnms.size(); idx++ )
    {
	uiSeisSel::Setup sssu( is2d, false );
	if ( seisinpnms.size()>1 )
	{
	    BufferString displaydatanm = seisinpnms.get(idx);
	    displaydatanm.quote(false);
	    sssu.seltxt( uiStrings::phrInput(toUiString("%1 %2")
			   .arg(uiStrings::sSeisObjName(is2d, !is2d, false))
			   .arg(displaydatanm ) ) );
	}
	else
	    sssu.seltxt( uiStrings::phrInput(
                                uiStrings::sSeisObjName(is2d, !is2d, false)) );
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
	    steerdatanm.quote(false);
	    tmpstr = uiStrings::phrInput(toUiString("%1 %2")
					   .arg(uiStrings::sSteering())
					   .arg(steerdatanm) );
	}
	else
	    tmpstr = uiStrings::phrInput( uiStrings::sSteering() );

	steerinpflds_ += new uiSteerCubeSel( this, is2d, true, tmpstr );
    }

    for ( int idx=1; idx<seisinpflds_.size(); idx++ )
	seisinpflds_[idx]->attach( alignedBelow, seisinpflds_[idx-1] );

    if ( steerinpflds_.size() && steerinpflds_[0] && seisinpflds_.size() )
	steerinpflds_[0]->attach( alignedBelow,
				  seisinpflds_[seisinpflds_.size()-1] );

    for ( int idx=1; idx<steerinpflds_.size(); idx++ )
	steerinpflds_[idx]->attach( alignedBelow, steerinpflds_[idx-1] );
}


#define mErrRetSelInp() \
    { \
	uiMSG().error( errmsg ); \
	return false; \
    }


bool uiAttrInpDlg::acceptOK()
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


const char* uiAttrInpDlg::getSeisRef( int idx ) const
{
    return seisinpflds_.size()>idx && seisinpflds_[idx]
		? seisinpflds_[idx]->getInput() : 0;
}


const char* uiAttrInpDlg::getSteerRef( int idx ) const
{
    return steerinpflds_.size()>idx && steerinpflds_[idx]
		? steerinpflds_[idx]->getInput() : 0;
}


DBKey uiAttrInpDlg::getSeisKey( int idx ) const
{
    if ( seisinpflds_.size()<=idx || !seisinpflds_[idx] )
	return DBKey::getInvalid();

    const IOObj* ioobj = seisinpflds_[idx]->ioobj( true );
    if ( !ioobj )
	return DBKey::getInvalid();

    return ioobj->key();
}


DBKey uiAttrInpDlg::getSteerKey( int idx ) const
{
    if ( steerinpflds_.size()<=idx || !steerinpflds_[idx] )
	return DBKey::getInvalid();

    const IOObj* ioobj = steerinpflds_[idx]->ioobj( true );
    if ( !ioobj )
	return DBKey::getInvalid();

    return ioobj->key();
}
