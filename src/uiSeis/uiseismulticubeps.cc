/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uiseismulticubeps.h"

#include "uilistbox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uiioobjsel.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "uistrings.h"
#include "seismulticubeps.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "od_helpids.h"

class uiSeisMultiCubePSEntry
{ mODTextTranslationClass(uiSeisMultiCubePSEntry);
public:
	uiSeisMultiCubePSEntry( IOObj* i )
	    : ioobj_(i), comp_(0)	{}
	uiSeisMultiCubePSEntry( const uiSeisMultiCubePSEntry& i )
	    : ioobj_(i.ioobj_->clone()), comp_(i.comp_)	{}
	~uiSeisMultiCubePSEntry()		{ delete ioobj_; }

	IOObj*	ioobj_;
	int	comp_;
};


uiSeisMultiCubePS::uiSeisMultiCubePS( uiParent* p, const MultiID& ky )
	: uiDialog(p,uiDialog::Setup(
		   !ky.isUdf() ? tr("Edit/Create MultiCube Prestack data store")
			     : tr("Create MultiCube Prestack data store"),
		   mNoDlgTitle, mODHelpKey(mSeisMultiCubePSHelpID) ))
	, ctio_(*mMkCtxtIOObj(SeisPS3D))
	, cubefld_(0)
	, curselidx_(-1)
	, outfld_(0)
	, offsfld_(0)
	, compfld_(0)
{
    ctio_.ctxt_.forread_ = false;
    ctio_.ctxt_.fixTranslator( "MultiCube" );
    if ( !ky.isUdf() )
	ctio_.setObj( ky );
    else
	ctio_.setObj( nullptr );

    fillEntries();
    if ( entries_.isEmpty() )
    {
	new uiLabel( this, tr("No cubes found.\n\n"
                     "Please import 3D seismic data."));
	return;
    }

    uiListBox::Setup su1( OD::ChooseOnlyOne, tr("Available cubes"),
			 uiListBox::AboveMid );
    cubefld_ = new uiListBox( this, su1 );
    fillBox( cubefld_ );
    cubefld_->setPrefWidthInChar( 30 );
    cubefld_->selectionChanged.notify( mCB(this,uiSeisMultiCubePS,inputChg) );
    allcompfld_ = new uiCheckBox( this, tr("Use all attribute components") );
    allcompfld_->setSensitive( false );
    allcompfld_->attach( alignedBelow, cubefld_ );

    uiButtonGroup* bgrp = new uiButtonGroup( this, "Buttons", OD::Vertical );
    new uiToolButton( bgrp, uiToolButton::RightArrow,uiStrings::sAdd(),
				mCB(this,uiSeisMultiCubePS,addCube) );
    new uiToolButton( bgrp, uiToolButton::LeftArrow, tr("Don't use"),
				mCB(this,uiSeisMultiCubePS,rmCube) );
    bgrp->attach( centeredRightOf, cubefld_ );

    uiListBox::Setup su2( OD::ChooseOnlyOne, tr("Used cubes"),
			  uiListBox::AboveMid );
    selfld_ = new uiListBox( this, su2 );
    selfld_->attach( rightTo, cubefld_ );
    selfld_->attach( ensureRightOf, bgrp );
    selfld_->selectionChanged.notify( mCB(this,uiSeisMultiCubePS,selChg) );
    selfld_->setPrefWidthInChar( 30 );

    compfld_ = new uiComboBox( this, "Component" );
    compfld_->setHSzPol( uiObject::WideMax );
    compfld_->attach( alignedBelow, selfld_ );
    compfld_->setSensitive( false );

    uiSeparator* sep = new uiSeparator( this, "Hor sep", OD::Horizontal, false);
    sep->attach( stretchedBelow, compfld_ );
    sep->attach( ensureBelow, allcompfld_ );

    uiString offsetstr = tr("Offset (start/step) %1")
						.arg(SI().getUiXYUnitString());
    const Interval<float> offsets( 0, 100 );
    offsfld_ = new uiGenInput( this, offsetstr,
			       FloatInpIntervalSpec(offsets).setName("Offset"));
    offsfld_->setElemSzPol( uiObject::Small );
    offsfld_->attach( alignedBelow, bgrp );
    offsfld_->attach( ensureBelow, sep );

    ctio_.ctxt_.toselect_.allownonuserselectable_ = true;
    outfld_ = new uiIOObjSel( this, ctio_, uiStrings::sOutpDataStore() );
    outfld_->attach( alignedBelow, offsfld_ );

    if ( ctio_.ioobj_ )
	afterPopup.notify( mCB(this,uiSeisMultiCubePS,setInitial) );
}


uiSeisMultiCubePS::~uiSeisMultiCubePS()
{
    delete ctio_.ioobj_;
    deepErase( entries_ );
    deepErase( selentries_ );
    delete &ctio_;
}


const IOObj* uiSeisMultiCubePS::createdIOObj() const
{
    return ctio_.ioobj_;
}


void uiSeisMultiCubePS::fillEntries()
{
    const IODir iodir( ctio_.ctxt_.getSelKey() );
    PtrMan<IOObjContext> ctxt = Seis::getIOObjContext( Seis::Vol, true );
    const IODirEntryList del( iodir, *ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IODirEntry& de = *del[idx];
	if ( !de.ioobj_ || !de.ioobj_->isUserSelectable() )
	    continue;

	entries_ += new uiSeisMultiCubePSEntry( de.ioobj_->clone() );
    }
}


void uiSeisMultiCubePS::recordEntryData()
{
    if ( curselidx_ < 0 || selentries_.isEmpty() )
	return;
    if ( curselidx_ >= selentries_.size() )
	curselidx_ = selentries_.size() - 1;

    uiSeisMultiCubePSEntry& se = *selentries_[curselidx_];
    se.comp_ = compfld_ && compfld_->isEmpty() ? 0 : compfld_->currentItem();
}


void uiSeisMultiCubePS::setInitial( CallBacker* cb )
{
    if ( !ctio_.ioobj_ )
	return;

    uiString emsg;
    ObjectSet<MultiID> keys; TypeSet<float> offs; TypeSet<int> comps;
    if ( !MultiCubeSeisPSReader::readData(ctio_.ioobj_->fullUserExpr(false),
		keys,offs,comps,emsg) )
	{ uiMSG().error( emsg ); return; }

    for ( int idx=0; idx<keys.size(); idx++ )
    {
	IOObj* ioobj = IOM().get( *keys[idx] );
	if ( !ioobj )
	    continue;
	uiSeisMultiCubePSEntry* entry = new uiSeisMultiCubePSEntry( ioobj );
	entry->comp_ = comps[idx];
	selentries_ += entry;
    }

    deepErase( keys );
    curselidx_ = selentries_.size() - 1;
    fullUpdate();
}


void uiSeisMultiCubePS::inputChg( CallBacker* )
{
    const int cubeidx = cubefld_->currentItem();
    if ( !entries_.validIdx(cubeidx) )
	return;

    uiSeisMultiCubePSEntry* entry = entries_[cubeidx];
    SeisIOObjInfo ioobjinf( entry->ioobj_ );
    BufferStringSet compnms;
    ioobjinf.getComponentNames( compnms );
    const bool hascomps = compnms.size() > 1;
    allcompfld_->setChecked( hascomps );
    allcompfld_->setSensitive( hascomps );
}


void uiSeisMultiCubePS::selChg( CallBacker* cb )
{
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 || selidx >= selentries_.size() )
	return;

    const uiSeisMultiCubePSEntry& se = *selentries_[selidx];
    curselidx_ = selidx;
    setCompFld( se );
}


void uiSeisMultiCubePS::setCompFld( const uiSeisMultiCubePSEntry& se )
{
    compfld_->setEmpty();
    SeisIOObjInfo ioobjinf( se.ioobj_ );
    BufferStringSet compnms;
    ioobjinf.getComponentNames( compnms );
    const bool dodisp = compnms.size() > 1;
    compfld_->setSensitive( dodisp );
    if ( dodisp )
    {
	compfld_->addItems( compnms );
	compfld_->setCurrentItem( se.comp_ );
    }
}


void uiSeisMultiCubePS::addCube( CallBacker* )
{
    const int cubeidx = cubefld_->currentItem();
    if ( cubeidx < 0 ) return;
    recordEntryData();

    uiSeisMultiCubePSEntry* entry = entries_[cubeidx];
    if ( !allcompfld_->isChecked() )
	selentries_ += new uiSeisMultiCubePSEntry( *entry );
    else
    {
	SeisIOObjInfo ioobjinf( entry->ioobj_ );
	BufferStringSet compnms;
	ioobjinf.getComponentNames( compnms );
	if ( compnms.size() > 1 )
	{
	    for ( int idx=0; idx<compnms.size(); idx++ )
	    {
		uiSeisMultiCubePSEntry* selentry =
					new uiSeisMultiCubePSEntry( *entry );
		selentry->comp_ = idx;
		selentries_ += selentry;
	    }
	}
    }

    curselidx_ = selentries_.size() - 1;
    fullUpdate();
}


void uiSeisMultiCubePS::rmCube( CallBacker* )
{
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 ) return;

    uiSeisMultiCubePSEntry* entry = selentries_[selidx];
    selentries_ -= entry;
    delete entry;

    if ( curselidx_ >= selentries_.size() )
	curselidx_ = selentries_.size() - 1;
    fullUpdate();
}


void uiSeisMultiCubePS::fillBox( uiListBox* lb )
{
    const ObjectSet<uiSeisMultiCubePSEntry>& es
		= lb == cubefld_ ? entries_ : selentries_;
    lb->setEmpty();
    for ( int idx=0; idx<es.size(); idx++ )
	lb->addItem( es[idx]->ioobj_->uiName() );
}


void uiSeisMultiCubePS::fullUpdate()
{
    if ( selfld_->size() != selentries_.size() )
	fillBox( selfld_ );
    if ( cubefld_->size() != entries_.size() )
    {
	int cubeidx = cubefld_->currentItem();
	if ( cubeidx < 0 ) cubeidx = 0;
	fillBox( cubefld_ );
	if ( !cubefld_->isEmpty() )
	    cubefld_->setCurrentItem( cubeidx );
    }

    if ( curselidx_ >= 0 )
    {
	uiSeisMultiCubePSEntry& se = *selentries_[curselidx_];
	setCompFld( se );
    }
    else
    {
	compfld_->setEmpty();
	compfld_->setSensitive( false );
    }

    selfld_->setCurrentItem( curselidx_ );
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiSeisMultiCubePS::acceptOK( CallBacker* )
{
    if ( entries_.isEmpty() )
	return true;

    if ( !outfld_ || !offsfld_ || !compfld_ )
	return true;

    recordEntryData();
    if ( !outfld_->commitInput() )
	mErrRet((outfld_->isEmpty()
	       ? uiStrings::phrSpecify(uiStrings::phrOutput(uiStrings::sName()))
	       : uiString::emptyString()))

    SamplingData<float> offset( offsfld_->getFValue(0),
				offsfld_->getFValue(1) );
    if ( offset.isUdf() )
    {
	uiMSG().error( tr("Please provide values for the offset start/step") );
	return false;
    }

    ObjectSet<MultiID> keys; TypeSet<float> offs; TypeSet<int> comps;
    for ( int idx=0; idx<selentries_.size(); idx++ )
    {
	const uiSeisMultiCubePSEntry& entry = *selentries_[idx];
	keys += new MultiID( entry.ioobj_->key() );
	offs += offset.atIndex( idx );
	comps += entry.comp_;
    }

    uiString emsg;
    const bool ret = MultiCubeSeisPSReader::writeData(
		ctio_.ioobj_->fullUserExpr(false), keys, offs, comps, emsg );
    deepErase( keys );
    if ( !ret )
	mErrRet(emsg)

    return true;
}
