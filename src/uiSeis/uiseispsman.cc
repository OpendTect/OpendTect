/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseispsman.h"

#include "od_helpids.h"
#include "posinfo.h"
#include "seisioobjinfo.h"
#include "seispsioprov.h"

#include "uiprestkcopy.h"
#include "uiprestkmergedlg.h"
#include "uiseismulticubeps.h"
#include "uitoolbutton.h"
#include "zdomain.h"

mDefineInstanceCreatedNotifierAccess(uiSeisPreStackMan)

#define mHelpID is2d ? mODHelpKey(mSeisPrestackMan2DHelpID) \
		     : mODHelpKey(mSeisPrestackMan3DHelpID)
uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(createCaption(is2d),mNoDlgTitle,mHelpID)
		     .nrstatusflds(1).modal(false),
		   is2d ? SeisPS2DTranslatorGroup::ioContext()
		        : SeisPS3DTranslatorGroup::ioContext())
    , is2d_(is2d)
    , copybut_(0)
    , mergebut_(0)
    , editbut_(0)
{
    createDefaultUI( true );
    if ( !is2d )
    {
	copybut_ = addManipButton( "copyobj", uiStrings::phrCopy(
					uiStrings::phrData(tr("Store"))),
					mCB(this,uiSeisPreStackMan,copyPush) );
	mergebut_ = addManipButton( "mergeseis", uiStrings::phrMerge(
					uiStrings::phrData(tr("Stores"))),
					mCB(this,uiSeisPreStackMan,mergePush) );
	addManipButton( "mkmulticubeps",
			     tr("Create/Edit Multi-Cube data store"),
			     mCB(this,uiSeisPreStackMan,mkMultiPush) );
	editbut_ = addManipButton( "browseseis",
			tr("Change file/folder names in SEG-Y file %1"),
			mCB(this,uiSeisPreStackMan,editPush) );
    }

    mTriggerInstanceCreatedNotifier();
    selChg(0);
}


uiSeisPreStackMan::~uiSeisPreStackMan()
{
}



uiString uiSeisPreStackMan::createCaption( bool is2d )
{
    return is2d
        ? uiStrings::phrManage( SeisPS2DTranslatorGroup::sTypeName() )
        : uiStrings::phrManage( SeisPS3DTranslatorGroup::sTypeName());
}


#define mSetButToolTip(but,str1,deftt) \
    if ( !but->sensitive() ) \
	but->setToolTip( deftt ); \
    else \
	but->setToolTip( str1 ); \

void uiSeisPreStackMan::ownSelChg()
{
    if ( is2d_ )
	return;

    uiString tt,cursel;
    if ( curioobj_ )
	cursel = curioobj_->uiName();

    copybut_->setSensitive( curioobj_ );
    mergebut_->setSensitive( curioobj_ );
    mSetButToolTip(copybut_,tr("Make a copy of '%1'").arg(cursel),
		   tr("Copy data store"));
    BufferStringSet selnms;
    getChosen( selnms );
    if ( selnms.size() > 1 )
    {
	mSetButToolTip(mergebut_, toUiString(selnms.getDispString(2)),
		       uiStrings::phrMerge(uiStrings::phrData(tr("Store"))));
    }
    else
	mergebut_->setToolTip(uiStrings::phrMerge(uiStrings::phrData(
								tr("Store"))));

    const uiSeisPreStackMan::BrowserDef* bdef = getBrowserDef();
    editbut_->display( bdef );
    if ( bdef )
    {
	uiString bdeftt( bdef->tooltip_ );
	editbut_->setToolTip( bdeftt.arg(curioobj_->uiName()) );
    }
    else
	editbut_->setToolTip( uiString::emptyString() );
}


void uiSeisPreStackMan::mkFileInfo()
{
    BufferString txt;
    SeisIOObjInfo objinf( curioobj_ );
    if ( objinf.isOK() )
    {
	if ( is2d_ )
	{
	    BufferStringSet nms;
	    SPSIOPF().getLineNames( *curioobj_, nms );
	    txt.set( "Line" ).add( nms.size() != 1 ? "s: " : ": " )
		.add( nms.getDispString(3,false) );
	}
	else
	{
	    PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( *curioobj_ );
	    if ( rdr )
	    {
		const PosInfo::CubeData& cd = rdr->posData();
		txt.add( "Total number of gathers: " ).add( cd.totalSize() );
		StepInterval<int> rg; cd.getInlRange( rg );
		txt.add( "\nInline range: " )
			    .add( rg.start ).add( " - " ).add( rg.stop );
		if ( cd.haveInlStepInfo() )
		    { txt.add( " [" ).add( rg.step ).add( "]" ); }
		cd.getCrlRange( rg );
		txt.add( "\nCrossline range: " )
			    .add( rg.start ).add( " - " ).add( rg.stop );
		if ( cd.haveCrlStepInfo() )
		    { txt.add( " [" ).add( rg.step ).add( "]" ); }
	    }
	}
	txt.add( "\n" );

	TrcKeyZSampling cs;
	if ( objinf.getRanges(cs) )
	{
	    const bool zistm = objinf.isTime();
	    const ZDomain::Def& zddef = objinf.zDomainDef();
#	    define mAddZValTxt(memb) .add(zistm ? mNINT32(1000*memb) : memb)
	    txt.add(zddef.userName().getFullString()).add(" range ")
		.add(zddef.unitStr(true)).add(": ") mAddZValTxt(cs.zsamp_.start)
		.add(" - ") mAddZValTxt(cs.zsamp_.stop)
		.add(" [") mAddZValTxt(cs.zsamp_.step) .add("]\n");
	}
    }

    txt += getFileInfo();
    setInfo( txt );
}


void uiSeisPreStackMan::copyPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiPreStackCopyDlg dlg( this, key );
    dlg.go();
    fullUpdate( key );
}


void uiSeisPreStackMan::mergePush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiPreStackMergeDlg dlg( this );
    BufferStringSet selnms;
    getChosen( selnms );
    dlg.setInputIds( selnms );
    dlg.go();
    fullUpdate( key );
}


void uiSeisPreStackMan::mkMultiPush( CallBacker* )
{
    MultiID key, toedit;
    if ( curioobj_ )
    {
	key = curioobj_->key();
	if ( curioobj_->translator() == "MultiCube" )
	    toedit = key;
    }

    uiSeisMultiCubePS dlg( this, toedit );
    dlg.go();
    fullUpdate( key );
}


static ObjectSet<uiSeisPreStackMan::BrowserDef> browserdefs_;

int uiSeisPreStackMan::addBrowser( uiSeisPreStackMan::BrowserDef* bd )
{
    browserdefs_ += bd;
    return browserdefs_.size() - 1;
}


const uiSeisPreStackMan::BrowserDef* uiSeisPreStackMan::getBrowserDef() const
{
    if ( curioobj_ )
    {
	for ( int idx=0; idx<browserdefs_.size(); idx++ )
	{
	    const BrowserDef* bdef = browserdefs_[idx];
	    if ( bdef->name_ == curioobj_->translator() )
		return bdef;
	}
    }

    return 0;
}


void uiSeisPreStackMan::editPush( CallBacker* )
{
    const uiSeisPreStackMan::BrowserDef* bdef = getBrowserDef();
    if ( bdef )
    {
	CallBack cb( bdef->cb_ );
	cb.doCall( this );
    }
}
