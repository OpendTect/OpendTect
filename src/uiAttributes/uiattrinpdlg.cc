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
#include "ioman.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "oddirs.h"
#include "perthreadrepos.h"


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset, 
			    bool issteer, bool is2d, const char* prevrefnm )
    : uiDialog(p,uiDialog::Setup("Attribute set definition",
		       issteer ? "Select Steering input"
			       : "Select Seismic input",
				 "101.1.2"))
    , multiinpcube_(true)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
{
    BufferString infotxt( "Provide input for the following attributes: " );
    uiLabel* infolbl = new uiLabel( this, infotxt );

    BufferString txt;
    for ( int idx=0; idx<refset.size(); idx++ )
	{ txt += refset.get(idx); txt += "\n"; }
    
    uiTextEdit* txtfld = new uiTextEdit( this, "File Info", true );
    txtfld->setPrefHeightInChar( 10 );
    txtfld->setPrefWidthInChar( 40 );
    txtfld->setText( txt );
    txtfld->attach( alignedBelow, infolbl );

    BufferString seltext = issteer ? "Input Steering cube" : "Input Seismics";
    if ( prevrefnm )
    {
	seltext += "\n (replacing '";
	seltext += prevrefnm;
	seltext += "')";
    }

    if ( issteer )
    {
	steerinpfld_ = new uiSteerCubeSel( this, is2d, true, seltext.buf() );
	steerinpfld_->attach( alignedBelow, txtfld );
    }
    else
    {
	uiSeisSel::Setup sssu( is2d, false );
	sssu.seltxt( seltext.buf() );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	seisinpfld_ = new uiSeisSel( this, ioctxt, sssu );
	seisinpfld_->attach( alignedBelow, txtfld );
    }

}


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, bool hasseis, bool hassteer,
       			    bool is2d )
    : uiDialog(p,uiDialog::Setup("Attribute set definition",
	       hassteer ? (hasseis ? "Select Seismic & Steering input"
			: "Select Steering input") : "Select Seismic input",
				 "101.1.2"))
    , multiinpcube_(false)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
{
    uiSeisSel::Setup sssu( is2d, false );
    if ( hasseis )
    {
	sssu.seltxt( "Input Seismics" );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	seisinpfld_ = new uiSeisSel( this, ioctxt, sssu );
    }

    if ( hassteer )
    {
	steerinpfld_ = new uiSteerCubeSel( this, is2d, true, "Input Steering" );
	if ( hasseis )
	    steerinpfld_->attach( alignedBelow, seisinpfld_ );
    }
}


#define mErrRetSelInp() \
    { \
	uiMSG().error( "Please select the input for the attributes" ); \
	return false; \
    }

bool uiAttrInpDlg::acceptOK( CallBacker* )
{
    if ( steerinpfld_ && !steerinpfld_->commitInput() && !multiinpcube_ )
	mErrRetSelInp();

    if ( seisinpfld_ && !seisinpfld_->commitInput() && !multiinpcube_ )
	mErrRetSelInp();

    return true;
}


uiAttrInpDlg::~uiAttrInpDlg()
{
}


const char* uiAttrInpDlg::getSeisRef() const
{
    return seisinpfld_ ? seisinpfld_->getInput() : 0;
}


const char* uiAttrInpDlg::getSteerRef() const
{
    return steerinpfld_ ? steerinpfld_->getInput() : 0;
}


const char* uiAttrInpDlg::getSeisKey() const
{
    LineKey lk;
    const IOObj* ioobj = seisinpfld_->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    if ( is2D() )
	lk.setAttrName( seisinpfld_->attrNm() );

    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}


const char* uiAttrInpDlg::getSteerKey() const
{
    static LineKey lk;
    const IOObj* ioobj = steerinpfld_->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    if ( is2D() )
	lk.setAttrName( steerinpfld_->attrNm() );

    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}
