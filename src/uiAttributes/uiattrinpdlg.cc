/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattrinpdlg.cc,v 1.20 2009-02-24 14:08:23 cvsbert Exp $";

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


static const char* seislbl[] = { "Select Seismics", 0 };
static const char* steerlbl[] = { "Select Steering cube", 0 };

uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset, 
			    bool issteer, bool is2d )
    : uiDialog(p,uiDialog::Setup("Attribute set definition",
		       issteer ? "Select Steering input"
			       : "Select Seismic input",
				 "101.1.2"))
    , ctio_(getCtxtIO(is2d))
    , ctiosteer_(*uiSteerCubeSel::mkCtxtIOObj(is2d,true))
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
    
    inpfld_ = new uiSeisSel( this, issteer ? ctiosteer_ : ctio_,
			uiSeisSel::Setup( is2d ? Seis::Line : Seis::Vol )
			.seltxt( issteer ? "Input Steering cube" 
					 : "Input Seismics") ); 
    inpfld_->attach( alignedBelow, txtfld );
}


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, bool hasseis, bool hassteer,
       			    bool is2d )
    : uiDialog(p,uiDialog::Setup("Attribute set definition",
	       hassteer ? (hasseis ? "Select Seismic & Steering input"
			: "Select Steering input") : "Select Seismic input",
				 "101.1.2"))
    , ctio_(getCtxtIO(is2d))
    , ctiosteer_(*uiSteerCubeSel::mkCtxtIOObj(is2d,true))
    , multiinpcube_(false)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
    , inpfld_(0)
{
    if ( hasseis )
    {
	seisinpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(
				     is2d ? Seis::Line : Seis::Vol)
				     .seltxt("Input Seismics Cube") ); 
    }

    if ( hassteer )
    {
	steerinpfld_ = new uiSeisSel( this, ctiosteer_, uiSeisSel::Setup(
				      is2d ? Seis::Line : Seis::Vol )
				      .seltxt("Input Steering cube") );
	if ( hasseis ) steerinpfld_->attach( alignedBelow, seisinpfld_ );
    }
}


CtxtIOObj& uiAttrInpDlg::getCtxtIO( bool is2d )
{
    IOM().setRootDir( GetDataDir() ); 
    IOM().to( SeisTrcTranslatorGroup::ioContext().getSelKey() ); 
    return *uiSeisSel::mkCtxtIOObj( is2d?Seis::Line:Seis::Vol, true );
}


#define mErrRetSelInp() \
    { \
	uiMSG().error( "Please select the input for the attributes" ); \
	return false; \
    }

bool uiAttrInpDlg::acceptOK( CallBacker* )
{
    if ( multiinpcube_ ) 
    {
	if ( !inpfld_->commitInput( false ) )
	    mErrRetSelInp()
    }
    else
    {
	if ( seisinpfld_ && !seisinpfld_->commitInput( false ) )
	    mErrRetSelInp()

	if ( steerinpfld_ && !steerinpfld_->commitInput( false ) )
	    mErrRetSelInp()
    }

    return true;
}


uiAttrInpDlg::~uiAttrInpDlg()
{
    delete ctio_.ioobj; delete &ctio_;
    delete ctiosteer_.ioobj; delete &ctiosteer_;
}


const char* uiAttrInpDlg::getUserRef() const
{
    return inpfld_ ? inpfld_->getInput() : 0;
}


const char* uiAttrInpDlg::getSeisRef() const
{
    return seisinpfld_ ? seisinpfld_->getInput() : 0;
}


const char* uiAttrInpDlg::getSteerRef() const
{
    return steerinpfld_ ? steerinpfld_->getInput() : 0;
}


const char* uiAttrInpDlg::getKey() const
{
    static LineKey lk;
    lk.setLineName( ctio_.ioobj->key() );
    if ( is2D() )
	lk.setAttrName( inpfld_->attrNm() );

    return lk;
}


const char* uiAttrInpDlg::getSeisKey() const
{
    static LineKey lk;
    lk.setLineName( ctio_.ioobj->key() );
    if ( is2D() )
	lk.setAttrName( seisinpfld_->attrNm() );
    return lk;
}


const char* uiAttrInpDlg::getSteerKey() const
{
    static LineKey lk;
    lk.setLineName( ctiosteer_.ioobj->key() );
    if ( is2D() )
	lk.setAttrName( steerinpfld_->attrNm() );
    return lk;
}
