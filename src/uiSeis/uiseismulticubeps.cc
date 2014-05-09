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
#include "survinfo.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "od_helpids.h"

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


uiSeisMultiCubePS::uiSeisMultiCubePS( uiParent* p, const char* ky )
	: uiDialog(p,uiDialog::Setup(
		   ky && *ky ? "Edit/Create MultiCube Prestack data store"
			     : "Create MultiCube Prestack data store",
		   mNoDlgTitle, mODHelpKey(mSeisMultiCubePSHelpID) ))
	, ctio_(*mMkCtxtIOObj(SeisPS3D))
	, cubefld_(0)
	, curselidx_(-1)
{
    ctio_.ctxt.forread = false;
    ctio_.ctxt.deftransl = ctio_.ctxt.toselect.allowtransls_ = "MultiCube";
    if ( ky && *ky )
	ctio_.setObj( MultiID(ky) );
    else
	ctio_.setObj( 0 );

    fillEntries();
    if ( entries_.isEmpty() )
    {
	new uiLabel( this, "No cubes found.\n\nPlease import 3D seismic data.");
	return;
    }

    uiLabeledListBox* cubesllb = new uiLabeledListBox( this, "Available cubes",
			    OD::ChooseOnlyOne, uiLabeledListBox::AboveMid );
    cubefld_ = cubesllb->box();
    fillBox( cubefld_ );
    cubefld_->setPrefWidthInChar( 30 );
    cubefld_->selectionChanged.notify( mCB(this,uiSeisMultiCubePS,inputChg) );
    allcompfld_ = new uiCheckBox( this, "Use all components" );
    allcompfld_->setSensitive( false );
    allcompfld_->attach( alignedBelow, cubesllb );

    uiButtonGroup* bgrp = new uiButtonGroup( this, "Buttons", OD::Vertical );
    new uiToolButton( bgrp, uiToolButton::RightArrow,"Add",
				mCB(this,uiSeisMultiCubePS,addCube) );
    new uiToolButton( bgrp, uiToolButton::LeftArrow, "Don't use",
				mCB(this,uiSeisMultiCubePS,rmCube) );
    bgrp->attach( centeredRightOf, cubesllb );

    uiLabeledListBox* selllb = new uiLabeledListBox( this, "Used cubes",
			    OD::ChooseOnlyOne, uiLabeledListBox::AboveMid );
    selfld_ = selllb->box();
    selllb->attach( rightTo, cubesllb );
    selllb->attach( ensureRightOf, bgrp );
    selfld_->selectionChanged.notify( mCB(this,uiSeisMultiCubePS,selChg) );
    selfld_->setPrefWidthInChar( 30 );

    BufferString offsetstr( "Offset ", SI().getXYUnitString() );
    offsfld_ = new uiGenInput( this, offsetstr,
			       FloatInpSpec().setName("Offset") );
    offsfld_->attach( alignedBelow, selllb );
    offsfld_->setElemSzPol( uiObject::Small );
    compfld_ = new uiComboBox( this, "Component" );
    compfld_->setHSzPol( uiObject::Medium );
    compfld_->attach( rightOf, offsfld_ );
    compfld_->display( false );

    uiSeparator* sep = new uiSeparator( this, "Hor sep", OD::Horizontal, false);
    sep->attach( stretchedBelow, offsfld_ );
    sep->attach( ensureBelow, allcompfld_ );

    outfld_ = new uiIOObjSel( this, ctio_, "Output data store" );
    outfld_->attach( alignedBelow, bgrp );
    outfld_->attach( ensureBelow, sep );

    if ( ctio_.ioobj )
	afterPopup.notify( mCB(this,uiSeisMultiCubePS,setInitial) );
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
    const IODir iodir( ctio_.ctxt.getSelKey() );
    const IODirEntryList del( iodir, SeisTrcTranslatorGroup::ioContext() );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IODirEntry& de = *del[idx];
	if ( !de.ioobj_ || !de.ioobj_->isReadDefault() ) continue;

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
    const float convfactor = SI().xyInFeet() ? mFromFeetFactorF : 1;
    se.offs_ = offsfld_->getfValue() * convfactor;
    se.comp_ = compfld_->isEmpty() ? 0 : compfld_->currentItem();
}


void uiSeisMultiCubePS::setInitial( CallBacker* cb )
{
    if ( !ctio_.ioobj )
	return;

    BufferString emsg;
    ObjectSet<MultiID> keys; TypeSet<float> offs; TypeSet<int> comps;
    if ( !MultiCubeSeisPSReader::readData(ctio_.ioobj->fullUserExpr(false),
		keys,offs,comps,emsg) )
	{ uiMSG().error( emsg ); return; }

    for ( int idx=0; idx<keys.size(); idx++ )
    {
	IOObj* ioobj = IOM().get( *keys[idx] );
	if ( !ioobj )
	    continue;
	uiSeisMultiCubePSEntry* entry = new uiSeisMultiCubePSEntry( ioobj );
	entry->offs_ = offs[idx];
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
		selentry->offs_ = mCast(float,10*idx);
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
			  entry.ioobj_->name(), "'");
	    return false;
	}
    }

    ObjectSet<MultiID> keys; TypeSet<float> offs; TypeSet<int> comps;
    for ( int idx=0; idx<selentries_.size(); idx++ )
    {
	const uiSeisMultiCubePSEntry& entry = *selentries_[idx];
	keys += new MultiID( entry.ioobj_->key() );
	offs += entry.offs_;
	comps += entry.comp_;
    }

    BufferString emsg;
    bool ret = MultiCubeSeisPSReader::writeData(
		    ctio_.ioobj->fullUserExpr(false), keys, offs, comps, emsg );
    deepErase( keys );
    if ( !ret )
	mErrRet(emsg)

    return true;
}
