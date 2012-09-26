/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "seismulticubeps.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "ioobj.h"

class uiSeisMultiCubePSEntry
{
public:
    	uiSeisMultiCubePSEntry( IOObj* i )
	    : ioobj_(i), offs_(mUdf(float)), comp_(0)	{}
    	uiSeisMultiCubePSEntry( const uiSeisMultiCubePSEntry& i )
	    : ioobj_(i.ioobj_->clone()), offs_(i.offs_), comp_(i.comp_)	{}
	~uiSeisMultiCubePSEntry()		{ delete ioobj_; }

	IOObj*	ioobj_;
	float	offs_;
	int	comp_;
};


uiSeisMultiCubePS::uiSeisMultiCubePS( uiParent* p )
	: uiDialog(p, uiDialog::Setup("MultiCube Pre-Stack data store",
		   "Create MultiCube Pre-Stack data store","103.1.7"))
	, ctio_(*mMkCtxtIOObj(SeisPS3D))
	, cubefld_(0)
	, curselidx_(-1)
{
    ctio_.ctxt.forread = false;
    ctio_.ctxt.deftransl = ctio_.ctxt.toselect.allowtransls_ = "MultiCube";

    fillEntries();
    if ( entries_.isEmpty() )
    {
	new uiLabel( this, "No cubes found.\n\nPlease import data first." );
	return;
    }

    uiLabeledListBox* cubesllb = new uiLabeledListBox( this, "Available cubes",
					false, uiLabeledListBox::AboveMid );
    cubefld_ = cubesllb->box();
    fillBox( cubefld_ );
    cubefld_->setPrefWidthInChar( 30 );

    uiButtonGroup* bgrp = new uiButtonGroup( this, "", true );
    new uiToolButton( bgrp, uiToolButton::RightArrow,"Add",
	    			mCB(this,uiSeisMultiCubePS,addCube) );
    new uiToolButton( bgrp, uiToolButton::LeftArrow, "Don't use",
	    			mCB(this,uiSeisMultiCubePS,rmCube) );
    bgrp->attach( centeredRightOf, cubesllb );

    uiLabeledListBox* selllb = new uiLabeledListBox( this, "Used cubes",
					false, uiLabeledListBox::AboveMid );
    selfld_ = selllb->box();
    selllb->attach( rightTo, cubesllb );
    selllb->attach( ensureRightOf, bgrp );
    selfld_->selectionChanged.notify( mCB(this,uiSeisMultiCubePS,selChg) );
    selfld_->setPrefWidthInChar( 30 );

    offsfld_ = new uiGenInput( this, "Offset", 
			       FloatInpSpec().setName("Offset") );
    offsfld_->attach( alignedBelow, selllb );
    offsfld_->setElemSzPol( uiObject::Small );
    compfld_ = new uiComboBox( this, "Component" );
    compfld_->setHSzPol( uiObject::Medium );
    compfld_->attach( rightOf, offsfld_ );
    compfld_->display( false );

    uiSeparator* sep = new uiSeparator( this, "Hor sep", true, false );
    sep->attach( stretchedBelow, offsfld_ );
    sep->attach( ensureBelow, cubesllb );

    outfld_ = new uiIOObjSel( this, ctio_, "Output data store" );
    outfld_->attach( alignedBelow, bgrp );
    outfld_->attach( ensureBelow, sep );
}


uiSeisMultiCubePS::~uiSeisMultiCubePS()
{
    delete ctio_.ioobj;
    deepErase( entries_ );
    deepErase( selentries_ );
    delete &ctio_;
}


const IOObj* uiSeisMultiCubePS::createdIOObj() const
{
    return ctio_.ioobj;
}


void uiSeisMultiCubePS::fillEntries()
{
    IOM().to( ctio_.ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), SeisTrcTranslatorGroup::ioContext() );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IODirEntry& de = *del[idx];
	if ( !de.ioobj || !de.ioobj->isReadDefault() ) continue;

	entries_ += new uiSeisMultiCubePSEntry( de.ioobj->clone() );
    }
}


void uiSeisMultiCubePS::recordEntryData()
{
    if ( curselidx_ < 0 || selentries_.isEmpty() )
	return;
    if ( curselidx_ >= selentries_.size() )
	curselidx_ = selentries_.size() - 1;

    uiSeisMultiCubePSEntry& se = *selentries_[curselidx_];
    se.offs_ = offsfld_->getfValue();
    se.comp_ = compfld_->isEmpty() ? 0 : compfld_->currentItem();
}


void uiSeisMultiCubePS::selChg( CallBacker* cb )
{
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 || selidx >= selentries_.size() )
	return;
    if ( cb ) recordEntryData();

    const uiSeisMultiCubePSEntry& se = *selentries_[selidx];
    curselidx_ = selidx;
    offsfld_->setValue( se.offs_ );
    setCompFld( se );
}


void uiSeisMultiCubePS::setCompFld( const uiSeisMultiCubePSEntry& se )
{
    compfld_->setEmpty();
    SeisIOObjInfo ioobjinf( se.ioobj_ );
    BufferStringSet compnms;
    ioobjinf.getComponentNames( compnms );
    const bool dodisp = compnms.size() > 1;
    compfld_->display( dodisp );
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
    selentries_ += new uiSeisMultiCubePSEntry( *entry );

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
	lb->addItem( es[idx]->ioobj_->name() );
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
	offsfld_->setValue( se.offs_ );
	setCompFld( se );
    }
    selfld_->setCurrentItem( curselidx_ );

}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiSeisMultiCubePS::acceptOK( CallBacker* )
{
    recordEntryData();
    if ( !outfld_->commitInput() )
	mErrRet(outfld_->isEmpty() ? "Please enter a name for the output" : 0)

    for ( int idx=0; idx<selentries_.size(); idx++ )
    {
	const uiSeisMultiCubePSEntry& entry = *selentries_[idx];
	if ( mIsUdf(entry.offs_) )
	{
	    uiMSG().error("Please provide the offset for '",
		    	  entry.ioobj_->name(),
			  "'");
	    return false;
	}
    }

    ObjectSet<MultiID> mids; TypeSet<float> offs; TypeSet<int> comps;
    for ( int idx=0; idx<selentries_.size(); idx++ )
    {
	const uiSeisMultiCubePSEntry& entry = *selentries_[idx];
	mids += new MultiID( entry.ioobj_->key() );
	offs += entry.offs_;
	comps += entry.comp_;
    }

    BufferString emsg;
    bool ret = MultiCubeSeisPSReader::writeData(
		    ctio_.ioobj->fullUserExpr(false), mids, offs, comps, emsg );
    deepErase( mids );
    if ( !ret )
	mErrRet(emsg)

    return true;
}
