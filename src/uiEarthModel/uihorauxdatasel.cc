/*+
 *  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *  AUTHOR   : Y.C. Liu
 *  DATE     : April 2010
-*/



#include "uihorauxdatasel.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "dbdir.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "od_helpids.h"

class uiHorizonAuxDataDlg: public uiDialog
{ mODTextTranslationClass(uiHorizonAuxDataDlg)
public:
uiHorizonAuxDataDlg( uiParent* p,
		     const uiHorizonAuxDataSel::HorizonAuxDataInfo& info )
    : uiDialog( p
    , uiDialog::Setup(toUiString("%1 %2 %3").arg(uiStrings::sHorizon())
					    .arg(uiStrings::sData())
					    .arg(uiStrings::sSelection())
					    , mNoDlgTitle
					    , mNoHelpKey ))
    , auxidx_( 0 )
    , selmid_( DBKey::getInvalid() )
    , auxinfo_( new uiHorizonAuxDataSel::HorizonAuxDataInfo(info) )
{
    uiGroup* grp = new uiGroup( this, "Group" );
    uiLabel* horlabel = new uiLabel(grp,
				    uiStrings::sHorizon(mPlural));
    horlistfld_ = new uiListBox( grp );
    horlistfld_->addItems( auxinfo_->hornms_ );
    horlistfld_->attach( ensureBelow, horlabel );
    horlistfld_->setHSzPol( uiObject::Wide );
    horlistfld_->selectionChanged.notify(
	    mCB(this,uiHorizonAuxDataDlg,selChg) );

    datalistfld_ = new uiListBox( grp );
    datalistfld_->attach( rightTo, horlistfld_ );
    datalistfld_->setHSzPol( uiObject::Wide );
    datalistfld_->selectionChanged.notify(
	    mCB(this, uiHorizonAuxDataDlg, auxidxChg) );

    uiLabel* directlabel = new uiLabel( grp, tr("Available data") );
    directlabel->attach( alignedAbove, datalistfld_ );
    directlabel->attach( rightTo, horlabel );
}

~uiHorizonAuxDataDlg()
{ delete auxinfo_; }

const DBKey&	selected() const
{ return selmid_; }

int dataidx()
{ return auxidx_; }


void setSelection( const DBKey& mid, int auxidx )
{
    if ( auxidx_==auxidx && selmid_==mid )
	return;

    auxidx_ = auxidx;
    if ( auxidx_>=0 )
	datalistfld_->setCurrentItem( auxidx_ );

    const bool hchanged = selmid_.isValid() && selmid_ != mid;
    selmid_ = mid;
    for ( int idx=0; idx<auxinfo_->dbkys_.size(); idx++ )
    {
	if ( auxinfo_->dbkys_[idx]==selmid_ )
	{
	    horlistfld_->setCurrentItem( idx );
	    return;
	}
    }

    if ( hchanged ) selChg( 0 );
}

protected:
void auxidxChg( CallBacker* )
{ auxidx_ = datalistfld_->currentItem(); }

void selChg( CallBacker* )
{
    const int hidx = horlistfld_->currentItem();
    if ( !auxinfo_->dbkys_.size() || hidx<0 )
	return;

    int oldidx =  auxinfo_->dbkys_[hidx]!=selmid_ ? 0 : auxidx_;
    selmid_ = auxinfo_->dbkys_[hidx];

    datalistfld_->setEmpty();
    datalistfld_->addItems( auxinfo_->auxdatanms_[hidx] );
    if ( oldidx>=0 )
	datalistfld_->setCurrentItem( oldidx );
}

    uiListBox*			horlistfld_;
    uiListBox*			datalistfld_;
    int				auxidx_;
    DBKey			selmid_;
    uiHorizonAuxDataSel::HorizonAuxDataInfo* auxinfo_;
};


uiHorizonAuxDataSel::HorizonAuxDataInfo::HorizonAuxDataInfo( bool load,
				uiParent* par )
{
    if ( !load )
	return;

    uiUserShowWait usw( par, uiStrings::sReadingData() );

    const DBDirEntryList horlist( mIOObjContext(EMHorizon3D) );
    for ( int idx=0; idx<horlist.size(); idx++ )
    {
	const IOObj& obj = horlist.ioobj( idx );
	EM::IOObjInfo eminfo( obj.key() );
	BufferStringSet attrnms;
	eminfo.getAttribNames( attrnms );
	if ( attrnms.size() )
	{
	    dbkys_ += obj.key();
	    hornms_.add( obj.name() );
	    auxdatanms_ += attrnms;
	}
    }
}


uiHorizonAuxDataSel::HorizonAuxDataInfo::HorizonAuxDataInfo(
	const HorizonAuxDataInfo& n )
{
    hornms_ = n.hornms_;
    dbkys_ = n.dbkys_;
    auxdatanms_ = n.auxdatanms_;
}


uiHorizonAuxDataSel::HorizonAuxDataInfo::~HorizonAuxDataInfo()
{
    dbkys_.erase();
    auxdatanms_.erase();
}


uiHorizonAuxDataSel::uiHorizonAuxDataSel( uiParent* p, const DBKey& mid,
	int auxidx, const HorizonAuxDataInfo* auxinfo )
    : uiGroup( p )
    , selmid_( mid )
    , auxidx_( auxidx )
    , nrhorswithdata_( 0 )
{
    horfld_ = new uiGenInput( this, uiStrings::sHorizon(), StringInpSpec());
    selbut_ = new uiPushButton( this, uiStrings::sSelect(),
	    mCB(this,uiHorizonAuxDataSel,selCB), false );
    selbut_->attach( rightOf, horfld_ );

    StringListInpSpec str;
    PtrMan<IOObj> obj = mid.getIOObj();
    const bool hasobj = mid.isValid() && obj;
    if ( hasobj )
    {
	EM::IOObjInfo eminfo( mid );
	BufferStringSet attrnms;
	eminfo.getAttribNames( attrnms );
	for ( int idx=0; idx<attrnms.size(); idx++ )
	    str.addString( toUiString(attrnms.get(idx)) );

	horfld_->setText( obj->name() );
    }

    auxfld_ = new uiGenInput( this, uiStrings::sHorizonData(), str );
    auxfld_->attach( rightOf, selbut_ );
    auxfld_->setPrefWidthInChar( 60 );
    auxfld_->valuechanged.notify(
	    mCB(this,uiHorizonAuxDataSel,auxidxChg) );
    if ( hasobj && auxidx>=0 ) auxfld_->setValue(auxidx);

    setHAlignObj( horfld_ );
    if ( auxinfo )
    {
	dlg_ = new uiHorizonAuxDataDlg( p, *auxinfo );
	nrhorswithdata_ = auxinfo->dbkys_.size();
    }
    else
    {
	HorizonAuxDataInfo ainfo( true );
	dlg_ = new uiHorizonAuxDataDlg( p, ainfo );
	nrhorswithdata_ = ainfo.dbkys_.size();
    }
}


void uiHorizonAuxDataSel::auxidxChg( CallBacker* )
{ auxidx_ = auxfld_->getIntValue(); }


void uiHorizonAuxDataSel::selCB( CallBacker* )
{
    dlg_->setSelection( selmid_, auxidx_ );
    dlg_->go();

    selmid_ = dlg_->selected();
    PtrMan<IOObj> obj = selmid_.getIOObj();
    if ( !obj )
	return;

    horfld_->setText( obj->name() );

    EM::IOObjInfo eminfo( selmid_ );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );

    StringListInpSpec str;
    for ( int idx=0; idx<attrnms.size(); idx++ )
	str.addString( toUiString(attrnms.get(idx)) );
    auxfld_->clear();
    auxfld_->newSpec( str, 0 );

    auxidx_ = dlg_->dataidx();
    auxfld_->setValue( auxidx_ );
}
