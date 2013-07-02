/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthexport.h"

#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "stratsynth.h"
#include "zdomain.h"


class uiSSOutSel : public uiCheckedCompoundParSel
{
public:

uiSSOutSel( uiParent* p, const char* seltxt, const BufferStringSet& nms )
    : uiCheckedCompoundParSel( p, seltxt, false, "&Select" )
    , nms_(nms)
    , nm_(seltxt)
{
    for ( int idx=0; idx<nms_.size(); idx++ )
	selidxs_ += idx;
    butPush.notify( mCB(this,uiSSOutSel,selItems) );
}

void selItems( CallBacker* )
{
    uiDialog::Setup su( BufferString("Select ",nm_), mNoDlgTitle, mTODOHelpID );
    uiDialog dlg( parent(), su );
    uiListBox* lb = new uiListBox( &dlg, nm_ );
    lb->addItems( nms_ );
    lb->setItemsCheckable( true );
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	lb->setItemChecked( selidxs_[idx], true );
    if ( dlg.go() )
    {
	selidxs_.erase();
	for ( int idx=0; idx<lb->size(); idx++ )
	    if ( lb->isItemChecked(idx) ) selidxs_ += idx;
    }
}

virtual BufferString getSummary() const
{
    BufferString ret;
    const int sz = nms_.size();
    const int selsz = selidxs_.size();

    if ( sz < 1 )
	ret = "<None available>";
    else if ( selsz == 0 )
	ret = "<None selected>";
    else if ( selsz == 1 )
	ret = nms_.get( selidxs_[0] );
    else
    {
	if ( sz == selsz )
	    ret = "<All>: ";
	ret.add( selsz ).add( " selected" );
    }
    
    return ret;
}

    const BufferString	nm_;
    const BufferStringSet nms_;
    TypeSet<int>	selidxs_;

    uiListBox*		listfld_;


};


uiStratSynthExport::uiStratSynthExport( uiParent* p, const StratSynth& ss )
    : uiDialog(p,uiDialog::Setup("Export synthetic seismics and horizons",
				 getWinTitle(ss), mTODOHelpID) )
    , ss_(ss)
{
    crnewfld_ = new uiGenInput( this, "Mode",
			     BoolInpSpec(true,"Create New","Use existing") );
    crnewfld_->valuechanged.notify( mCB(this,uiStratSynthExport,crNewChg) );

    uiSeisSel::Setup sssu( Seis::Line );
    sssu.enabotherdomain( false ).zdomkey( ZDomain::sKeyTime() )
	.selattr( false ).allowsetdefault( false );
    linesetsel_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,false),
	    			sssu );
    linesetsel_->attach( alignedBelow, crnewfld_ );
    linenmsel_ = new uiSeis2DLineNameSel( this, false );
    linenmsel_->attach( alignedBelow, linesetsel_ );

    BufferStringSet nms;
    nms.add( "Post-stack 1" ); nms.add( "Post-stack 2" );
    poststcksel_ = new uiSSOutSel( this, "Post-stack line data", nms );
    poststcksel_->attach( alignedBelow, linenmsel_ );
    nms.erase(); nms.add( "Level 1" ); nms.add( "Level 2" ).add( "Level 3" );
    horsel_ = new uiSSOutSel( this, "2D horizons", nms );
    horsel_->attach( alignedBelow, poststcksel_ );
    nms.erase(); nms.add( "Pre-stack 1" );
    prestcksel_ = new uiSSOutSel( this, "Pre-stack data", nms );
    prestcksel_->attach( alignedBelow, horsel_ );

    postFinalise().notify( mCB(this,uiStratSynthExport,crNewChg) );
}


uiStratSynthExport::~uiStratSynthExport()
{
}


BufferString uiStratSynthExport::getWinTitle( const StratSynth& ss ) const
{
    return BufferString( "TODO: create nice window title" );
}


void uiStratSynthExport::crNewChg( CallBacker* )
{
    // const bool iscreate = crnewfld_->getBoolValue();
}


void uiStratSynthExport::selOutputCB( CallBacker* cb )
{
    mDynamicCastGet(uiSSOutSel*,ssout,cb)
    if ( !ssout )
	uiMSG().error( "ouch, passed CallBacker is not the uiSSOutSel" );
}


bool uiStratSynthExport::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: complete and implement" );
    return false;
}
