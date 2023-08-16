/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrdescseted.h"

#include "ascstream.h"
#include "attribfactory.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsettr.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "ptrman.h"
#include "pickset.h"
#include "seistype.h"
#include "survinfo.h"
#include "settings.h"
#include "od_istream.h"

#include "uiattrdesced.h"
#include "uiattrgetfile.h"
#include "uiattribfactory.h"
#include "uiattrinpdlg.h"
#include "uiattrtypesel.h"
#include "uiattrvolout.h"
#include "uiautoattrdescset.h"
#include "uicombobox.h"
#include "uidesktopservices.h"
#include "uievaluatedlg.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uihelpview.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiselobjothersurv.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uistoredattrreplacer.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "od_helpids.h"


EvalParam::EvalParam( const char* lbl, const char* par1,
		      const char* par2, int idx )
    : label_(lbl)
    , par1_(par1)
    , par2_(par2)
    , pgidx_(idx)
    , evaloutput_(false)
{}


EvalParam::~EvalParam()
{}


uiAttrDescEdCreater::uiAttrDescEdCreater()
{}


uiAttrDescEdCreater::~uiAttrDescEdCreater()
{}


const char* uiAttribDescSetEd::sKeyUseAutoAttrSet = "dTect.Auto Attribute set";
const char* uiAttribDescSetEd::sKeyAuto2DAttrSetID = "2DAttrset.Auto ID";
const char* uiAttribDescSetEd::sKeyAuto3DAttrSetID = "3DAttrset.Auto ID";

BufferString uiAttribDescSetEd::nmprefgrp_( "" );
static const char* sKeyNotSaved = "<not saved>";

static bool prevsavestate = true;

using namespace Attrib;

uiAttribDescSetEd::uiAttribDescSetEd( uiParent* p, DescSetMan* adsm,
				      const char* prefgrp, bool attrsneedupdt )
    : uiDialog(p,uiDialog::Setup( adsm && adsm->is2D() ? tr("Attribute Set 2D")
					: tr("Attribute Set 3D"),mNoDlgTitle,
					mODHelpKey(mAttribDescSetEdHelpID) )
	.savebutton(true).savetext(tr("Save on Close"))
	.menubar(true).modal(false))
    , inoutadsman_(adsm)
    , userattrnames_(*new BufferStringSet)
    , setctio_(*mMkCtxtIOObj(AttribDescSet))
    , prevdesc_(0)
    , attrset_(0)
    , dirshowcb(this)
    , evalattrcb(this)
    , crossevalattrcb(this)
    , xplotcb(this)
    , applycb(this)
    , adsman_(0)
    , updating_fields_(false)
    , attrsneedupdt_(attrsneedupdt)
    , zdomaininfo_(0)
{
    setOkCancelText( uiStrings::sClose(), uiString::emptyString() );
    setctio_.ctxt_.toselect_.dontallow_.set( sKey::Type(),
					   adsm->is2D() ? "3D" : "2D" );

    createMenuBar();
    createToolBar();
    createGroups();
    attrtypefld_->setGrp( prefgrp ? prefgrp : nmprefgrp_.buf() );

    init();
}


void uiAttribDescSetEd::setZDomainInfo( const ZDomain::Info& info )
{
    delete zdomaininfo_; zdomaininfo_ = new ZDomain::Info(info);
    for ( int idx=0; idx<desceds_.size(); idx++ )
	if ( desceds_[idx] ) desceds_[idx]->setZDomainInfo( zdomaininfo_ );
}


const ZDomain::Info* uiAttribDescSetEd::getZDomainInfo() const
{ return zdomaininfo_; }


#define mInsertMnuItem( mnu, txt, func, fnm ) \
{ \
    uiAction* itm = new uiAction(txt,mCB(this,uiAttribDescSetEd,func),fnm);\
    mnu->insertAction( itm ); \
}

#define mInsertMnuItemNoIcon( mnu, txt, func ) \
{ \
    uiAction* itm = new uiAction(txt,mCB(this,uiAttribDescSetEd,func));\
    mnu->insertAction( itm ); \
}

#define mInsertItem( txt, func, fnm ) mInsertMnuItem(filemnu,txt,func,fnm)
#define mInsertItemNoIcon( txt, func ) mInsertMnuItemNoIcon(filemnu,txt,func)

void uiAttribDescSetEd::createMenuBar()
{
    uiMenuBar* menubar = menuBar();
    if( !menubar )
	{ pErrMsg("huh?"); return; }

    uiMenu* filemnu = new uiMenu( this, uiStrings::sFile() );
    mInsertItem( m3Dots(tr("New set")), newSet, "new" );
    mInsertItem( m3Dots(tr("Open set")), openSet, "open" );
    mInsertItem( m3Dots(tr("Save set")), savePush, "save" );
    mInsertItem( m3Dots(tr("Save set as")), saveAsPush, "saveas" );
    mInsertItemNoIcon( m3Dots(tr("Auto Load Attribute Set")), autoSet );
    mInsertItemNoIcon( m3Dots(tr("Change attribute input(s)")), changeInput );
    filemnu->insertSeparator();
    mInsertItem( m3Dots(tr("Open Default set")), defaultSet, "defset" );
    uiMenu* impmnu = new uiMenu( this, uiStrings::sImport() );
    mInsertMnuItem( impmnu, m3Dots(tr("From other Survey")),
		    importSet, "impset" );
    mInsertMnuItemNoIcon( impmnu, m3Dots(tr("From File")), importFile );
    mInsertItem( m3Dots(tr("Reconstruct from job file")), job2Set, "job2set" );
    mInsertItemNoIcon( m3Dots(tr("Import set from Seismic")), importFromSeis );

    filemnu->addMenu( impmnu );
    menubar->addMenu( filemnu );
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, tip, mCB(this,uiAttribDescSetEd,func) )

void uiAttribDescSetEd::createToolBar()
{
    toolbar_ = new uiToolBar( this, tr("AttributeSet tools") );
    mAddButton( "new", newSet, tr("New attribute set") );
    mAddButton( "open", openSet, tr("Open attribute set") );
    mAddButton( "defset", defaultSet, tr("Open default attribute set") );
    mAddButton( "impset", importSet,
		tr("Import attribute set from other survey") );
    mAddButton( "job2set", job2Set, tr("Reconstruct set from job file") );
    mAddButton( "save", savePush, tr("Save attribute set") );
    mAddButton( "saveas", saveAsPush, tr("Save attribute set as") );
    toolbar_->addSeparator();
    mAddButton( "evalattr", evalAttribute, tr("Evaluate attribute") );
    mAddButton( "evalcrossattr",crossEvalAttrs,tr("Cross attributes evaluate"));
    mAddButton( "xplot", crossPlot, tr("Cross-Plot attributes") );
    const int dotidx = mAddButton( "dot", exportToDotCB, tr("View as graph") );
    uiMenu* mnu = new uiMenu( nullptr, uiString::emptyString() );
    mnu->insertAction( new uiAction(m3Dots(tr("Graphviz Installation")),
	mCB(this,uiAttribDescSetEd,dotPathCB)) );
    toolbar_->setButtonMenu( dotidx, mnu );
}


void uiAttribDescSetEd::createGroups()
{
//  Left part
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    attrsetfld_ = new uiGenInput(leftgrp, tr("Attribute set"), StringInpSpec());
    attrsetfld_->setReadOnly( true );

    attrlistfld_ = new uiListBox( leftgrp, "Defined Attributes" );
    attrlistfld_->setStretch( 2, 2 );
    attrlistfld_->selectionChanged.notify( mCB(this,uiAttribDescSetEd,selChg) );
    attrlistfld_->attach( leftAlignedBelow, attrsetfld_ );

    moveupbut_ = new uiToolButton( leftgrp, uiToolButton::UpArrow,
				   uiStrings::sUp(),
				   mCB(this,uiAttribDescSetEd,moveUpDownCB) );
    moveupbut_->attach( centeredRightOf, attrlistfld_ );
    movedownbut_ = new uiToolButton( leftgrp, uiToolButton::DownArrow,
				     uiStrings::sDown(),
				     mCB(this,uiAttribDescSetEd,moveUpDownCB) );
    movedownbut_->attach( alignedBelow, moveupbut_ );
    sortbut_ = new uiToolButton( leftgrp, "sort", tr("Sort attributes"),
				 mCB(this,uiAttribDescSetEd,sortPush) );
    sortbut_->attach( alignedBelow, movedownbut_ );
    rmbut_ = new uiToolButton( leftgrp, "remove", tr("Remove selected"),
				mCB(this,uiAttribDescSetEd,rmPush) );
    rmbut_->attach( alignedBelow, sortbut_ );

//  Right part
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );

    uiGroup* degrp = new uiGroup( rightgrp, "DescEdGroup" );
    degrp->setStretch( 1, 1 );

    attrtypefld_ = new uiAttrTypeSel( degrp );
    attrtypefld_->selChg.notify( mCB(this,uiAttribDescSetEd,attrTypSel) );
    desceds_.allowNull();
    for ( int idx=0; idx<uiAF().size(); idx++ )
    {
	uiAttrDescEd::DomainType dt =
		(uiAttrDescEd::DomainType)uiAF().domainType( idx );
	if ( (dt == uiAttrDescEd::Depth && SI().zIsTime())
		|| (dt == uiAttrDescEd::Time && !SI().zIsTime()) )
	    continue;

	const bool is2d = inoutadsman_ ? inoutadsman_->is2D() : false;
	uiAttrDescEd::DimensionType dimtyp =
		(uiAttrDescEd::DimensionType)uiAF().dimensionType( idx );
	if ( (dimtyp == uiAttrDescEd::Only3D && is2d)
		|| (dimtyp == uiAttrDescEd::Only2D && !is2d) )
	    continue;

	const char* attrnm = uiAF().getDisplayName(idx);
	attrtypefld_->add( uiAF().getGroupName(idx), attrnm );
	uiAttrDescEd* de = uiAF().create( degrp, attrnm, is2d, true );
	if ( zdomaininfo_ )
	    de->setZDomainInfo( zdomaininfo_ );

	desceds_ += de;
	de->attach( alignedBelow, attrtypefld_ );
    }
    attrtypefld_->update();
    degrp->setHAlignObj( attrtypefld_ );

    helpbut_ = new uiToolButton( degrp, "contexthelp", uiStrings::sHelp(),
				mCB(this,uiAttribDescSetEd,helpButPush) );
    helpbut_->attach( rightTo, attrtypefld_ );
    uiToolButton* matrixbut = new uiToolButton( degrp, "attributematrix",
	tr("Attribute Matrix"), mCB(this,uiAttribDescSetEd,showMatrix) );
    matrixbut->attach( rightTo, helpbut_ );
    videobut_ = new uiToolButton( degrp, "video", uiStrings::sVideo(),
				mCB(this,uiAttribDescSetEd,videoButPush) );
    videobut_->attach( rightTo, matrixbut );

    attrnmfld_ = new uiGenInput( rightgrp, uiStrings::sAttribName() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, degrp );
    attrnmfld_->updateRequested.notify( mCB(this,uiAttribDescSetEd,addPush) );

    addbut_ = new uiPushButton( rightgrp, tr("Add as new"), true );
    addbut_->attach( rightTo, attrnmfld_ );
    addbut_->activated.notify( mCB(this,uiAttribDescSetEd,addPush) );

    dispbut_ = new uiToolButton( rightgrp, "showattrnow",
	tr("Recalculate this attribute on selected element"),
	mCB(this,uiAttribDescSetEd,directShow) );
    dispbut_->attach( rightTo, addbut_ );

    uiToolButton* procbut = new uiToolButton( rightgrp, "seisout",
	tr("Process this attribute"),
	mCB(this,uiAttribDescSetEd,procAttribute) );
    procbut->attach( rightTo, dispbut_ );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


#define mUnsetAuto \
    SI().getPars().removeWithKey( autoidkey ); \
    SI().savePars()

void uiAttribDescSetEd::init()
{
    delete attrset_;
    attrset_ = new Attrib::DescSet( *inoutadsman_->descSet() );
    delete adsman_;
    adsman_ = new DescSetMan( inoutadsman_->is2D(), attrset_, false );
    adsman_->fillHist();
    adsman_->setSaved( inoutadsman_->isSaved() );

    setid_ = inoutadsman_->attrsetid_;
    IOM().to( setctio_.ctxt_.getSelKey() );
    setctio_.setObj( IOM().get(setid_) );
    bool autoset = false;
    MultiID autoid;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, autoset );
    const char* autoidkey = is2D() ? uiAttribDescSetEd::sKeyAuto2DAttrSetID
				   : uiAttribDescSetEd::sKeyAuto3DAttrSetID;
    if ( autoset && SI().pars().get(autoidkey,autoid) && autoid != setid_ )
    {
	uiString msg = tr("The Attribute-set selected for Auto-load"
			  " is no longer valid.\n Load another now?");

	if ( uiMSG().askGoOn( msg ) )
	{
	    BufferStringSet attribfiles;
	    BufferStringSet attribnames;
	    getDefaultAttribsets( attribfiles, attribnames );
	    uiAutoAttrSetOpen dlg( this, attribfiles, attribnames );
	    if ( dlg.go() )
	    {
		if ( dlg.isUserDef() )
		{
		    IOObj* ioobj = dlg.getObj();
		    if ( dlg.isAuto() )
		    {
			const MultiID id =
				ioobj ? ioobj->key() : MultiID::udf();
			SI().getPars().set( autoidkey, id );
			SI().savePars();
		    }
		    else
		    {
			mUnsetAuto;
		    }

		    openAttribSet( ioobj );
		}
		else
		{
		    const char* filenm = dlg.getAttribfile();
		    const char* attribnm = dlg.getAttribname();
		    importFromFile( filenm );
		    attrsetfld_->setText( attribnm );
		    mUnsetAuto;
		}
	    }
	    else
	    {
		mUnsetAuto;
	    }
	}
	else
	{
	    mUnsetAuto;
	}
    }
    else
    {
	const BufferString txt = setctio_.ioobj_ ? setctio_.ioobj_->name().buf()
						: sKeyNotSaved;
	attrsetfld_->setText( txt );
    }

    cancelsetid_ = setid_;
    newList(0);

    setSaveButtonChecked( prevsavestate );
    setButStates();
}


uiAttribDescSetEd::~uiAttribDescSetEd()
{
    delete &userattrnames_;
    delete &setctio_;
    delete adsman_;
}


void uiAttribDescSetEd::setDescSetMan( DescSetMan* adsman )
{
    inoutadsman_ = adsman;
    init();
}


void uiAttribDescSetEd::setSensitive( bool yn )
{
    topGroup()->setSensitive( yn );
    menuBar()->setSensitive( yn );
    toolbar_->setSensitive( yn );
}



#define mErrRetFalse(s) { uiMSG().error( s ); return false; }
#define mErrRet(s) { uiMSG().error( s ); return; }
#define mErrRetNull(s) { uiMSG().error( s ); return 0; }


void uiAttribDescSetEd::attrTypSel( CallBacker* )
{
    updateFields( false );
}


void uiAttribDescSetEd::selChg( CallBacker* )
{
    if ( updating_fields_ ) return;
	// Fix for continuous call during re-build of list

    doCommit( true );
    updateFields();
    prevdesc_ = curDesc();
    setButStates();
    applycb.trigger();
}


void uiAttribDescSetEd::savePush( CallBacker* )
{
    removeNotUsedAttr();
    doSave( true );
}


void uiAttribDescSetEd::saveAsPush( CallBacker* )
{
    removeNotUsedAttr();
    doSave( false );
}


bool uiAttribDescSetEd::doSave( bool endsave )
{
    doCommit();
    setctio_.ctxt_.forread_ = false;
    IOObj* oldioobj = setctio_.ioobj_;
    bool needpopup = !oldioobj || !endsave;
    if ( needpopup )
    {
	uiIOObjSelDlg dlg( this, setctio_ );
	if ( !dlg.go() || !dlg.ioObj() ) return false;

	setctio_.ioobj_ = 0;
	setctio_.setObj( dlg.ioObj()->clone() );
    }

    if ( !doSetIO( false ) )
    {
	if ( oldioobj != setctio_.ioobj_ )
	    setctio_.setObj( oldioobj );
	return false;
    }

    if ( oldioobj != setctio_.ioobj_ )
	delete oldioobj;
    setid_ = setctio_.ioobj_->key();
    attrsetfld_->setText( setctio_.ioobj_->name() );
    adsman_->setSaved( true );
    return true;
}


void uiAttribDescSetEd::autoSet( CallBacker* )
{
    const bool is2d = is2D();
    uiAutoAttrSelDlg dlg( this, is2d );
    if ( dlg.go() )
    {
	const bool douse = dlg.useAuto();
	IOObj* ioobj = dlg.getObj();
	const MultiID id( ioobj ? ioobj->key() : MultiID("") );
	Settings::common().setYN(uiAttribDescSetEd::sKeyUseAutoAttrSet, douse);
	Settings::common().write();
	IOPar& par = SI().getPars();
	is2d ? par.set( uiAttribDescSetEd::sKeyAuto2DAttrSetID, id )
	     : par.set( uiAttribDescSetEd::sKeyAuto3DAttrSetID, id );
	SI().savePars();
	if ( dlg.loadAuto() )
	{
	    if ( !offerSetSave() ) return;
	    openAttribSet( ioobj );
	}
    }
}


void uiAttribDescSetEd::revPush( CallBacker* )
{
    updateFields();
}


void uiAttribDescSetEd::addPush( CallBacker* )
{
    Desc* newdesc = createAttribDesc();
    if ( !newdesc ) return;
    if ( !attrset_->addDesc(newdesc).isValid() )
	{ uiMSG().error( attrset_->errMsg() ); newdesc->unRef(); return; }

    newList( attrdescs_.size() );
    adsman_->setSaved( false );
    applycb.trigger();
}


Attrib::Desc* uiAttribDescSetEd::createAttribDesc( bool checkuref )
{
    BufferString newnmbs( attrnmfld_->text() );
    char* newnm = newnmbs.getCStr();
    mSkipBlanks( newnm );
    removeTrailingBlanks( newnm );
    if ( checkuref && !validName(newnm) ) return 0;

    uiAttrDescEd* curde = curDescEd();
    if ( !curde )
	mErrRetNull( tr("Cannot add without a valid attribute type") )

    BufferString attribname = curde->attribName();
    if ( attribname.isEmpty() )
	attribname = uiAF().attrNameOf( attrtypefld_->attr() );

    Desc* newdesc = PF().createDescCopy( attribname );
    if ( !newdesc )
	mErrRetNull( toUiString("Cannot create attribdesc") )

    newdesc->ref();
    newdesc->setDescSet( attrset_ );
    newdesc->setUserRef( newnm );
    uiString res = curde->commit( newdesc );
    if ( !res.isEmpty() )
    {
	newdesc->unRef();
	mErrRetNull( res );
    }

    return newdesc;
}


void uiAttribDescSetEd::helpButPush( CallBacker* )
{
    uiAttrDescEd* curdesced = curDescEd();
    HelpProvider::provideHelp( curdesced->helpKey() );
}


void uiAttribDescSetEd::videoButPush( CallBacker* )
{
    uiAttrDescEd* curdesced = curDescEd();
    HelpProvider::provideHelp( curdesced->videoKey() );
}


void uiAttribDescSetEd::rmPush( CallBacker* )
{
    Desc* curdesc = curDesc();
    if ( !curdesc ) return;

    BufferString depattribnm;
    if ( attrset_->isAttribUsed( curdesc->id(), depattribnm ) )
    {
	uiMSG().error( tr("Cannot remove this attribute. It is used\n"
			  "as input for another attribute called '%1'")
			.arg(depattribnm.buf()) );
	return;
    }

    const int curidx = attrdescs_.indexOf( curdesc );
    attrset_->removeDesc( attrset_->getID(*curdesc) );
    newList( curidx );
    removeNotUsedAttr();
    adsman_->setSaved( false );
    setButStates();
}


void uiAttribDescSetEd::moveUpDownCB( CallBacker* cb )
{
    Desc* curdesc = curDesc();
    if ( !curdesc ) return;

    const bool moveup = cb == moveupbut_;
    const int curidx = attrdescs_.indexOf( curdesc );
    attrset_->moveDescUpDown( attrset_->getID(*curdesc), moveup );
    newList( curidx );
    attrlistfld_->setCurrentItem( moveup ? curidx-1 : curidx+1 );
    adsman_->setSaved( false );
    setButStates();
}


void uiAttribDescSetEd::setButStates()
{
    const int selidx = attrlistfld_->currentItem();
    moveupbut_->setSensitive( selidx > 0 );
    movedownbut_->setSensitive( selidx < attrlistfld_->size()-1 );
    sortbut_->setSensitive( selidx > 0 );
    int size = attrlistfld_->size();
    sortbut_->setSensitive( size > 1);
}


void uiAttribDescSetEd::sortPush( CallBacker* )
{
    attrset_->sortDescSet();
    int size = attrdescs_.size();
    for ( int idx=0; idx<size; idx++ )
	newList( idx );
}


void uiAttribDescSetEd::handleSensitivity()
{
    bool havedescs = !attrdescs_.isEmpty();
    rmbut_->setSensitive( havedescs );
}


bool uiAttribDescSetEd::acceptOK( CallBacker* )
{
    if ( inoutadsman_ )
	inoutadsman_->setSaved( adsman_->isSaved() );

    if ( !curDesc() )
	return true;

    if ( !doCommit() || !doAcceptInputs() )
	return false;

    removeNotUsedAttr();
    if ( saveButtonChecked() && !doSave(true) )
	return false;

    prevsavestate = saveButtonChecked();
    nmprefgrp_ = attrtypefld_->group();
    applycb.trigger();
    return true;
}


bool uiAttribDescSetEd::rejectOK( CallBacker* )
{
    setid_ = cancelsetid_;
    return true;
}


void uiAttribDescSetEd::newList( int newcur )
{
    prevdesc_ = 0;
    updateUserRefs();
    // Fix for continuous call during re-build of list
    updating_fields_ = true;
    attrlistfld_->setEmpty();
    attrlistfld_->addItems( userattrnames_ );
    updating_fields_ = false;
    if ( newcur < 0 ) newcur = 0;
    if ( newcur >= attrlistfld_->size() ) newcur = attrlistfld_->size()-1;
    if ( !userattrnames_.isEmpty() )
    {
	attrlistfld_->setCurrentItem( newcur );
	prevdesc_ = curDesc();
    }
    updateFields();
    handleSensitivity();
    setButStates();
}


void uiAttribDescSetEd::setSelAttr( const char* attrnm )
{
    if ( !attrtypefld_ ) return;

    newSet(0);
    attrtypefld_->setAttr( attrnm );
    updateFields( false );
}


void uiAttribDescSetEd::updateFields( bool set_type )
{
    updating_fields_ = true;
    updateAttrName();
    Desc* desc = curDesc();
    uiAttrDescEd* curde = 0;
    if ( set_type )
    {
	BufferString typenm = desc ? desc->attribName().buf() : "RefTime";
	attrtypefld_->setAttr( uiAF().dispNameOf(typenm) );
	curde = curDescEd();
    }
    if ( !curde )
	curde = curDescEd();

    BufferString attribname = curde ? curde->attribName() : "";
    if ( attribname.isEmpty() )
	attribname = uiAF().attrNameOf( attrtypefld_->attr() );
    const bool isrightdesc = desc && attribname == desc->attribName();

    Desc* dummydesc = PF().createDescCopy( attribname );
    if ( !dummydesc )
	dummydesc = new Desc( "Dummy" );

    dummydesc->ref();
    dummydesc->setDescSet( attrset_ );
    for ( int idx=0; idx<desceds_.size(); idx++ )
    {
	uiAttrDescEd* de = desceds_[idx];
	if ( !de ) continue;

	if( !set_type )
	    de->setNeedInputUpdate();

	if ( curde == de )
	{
	    if ( !isrightdesc )
		dummydesc->ref();
	    de->setDesc( isrightdesc ? desc : dummydesc, adsman_ );
	}

	de->setDescSet( attrset_ );
	const bool dodisp = de == curde;
	de->display( dodisp );
    }

    dummydesc->unRef();
    updating_fields_ = false;

    bool hasvideo = false;
    if ( curDescEd() )
    {
	const HelpKey videokey = curDescEd()->videoKey();
	hasvideo = HelpProvider::hasHelp( videokey );
	if ( hasvideo )
	    videobut_->setToolTip( HelpProvider::description(videokey) );
    }
    videobut_->display( hasvideo );
}


bool uiAttribDescSetEd::doAcceptInputs()
{
    for ( int idx=0; idx<attrset_->size(); idx++ )
    {
	const DescID descid = attrset_->getID( idx );
	Desc* desc = attrset_->getDesc( descid );
	uiString errmsg = curDescEd()->errMsgStr( desc );
	if ( !errmsg.isEmpty() )
	{
	    if ( desc->isStored() )
		uiMSG().error( errmsg );
	    else
	    {
		const char* attribname = desc->userRef();
		uiString msg = tr("Input is not correct for attribute '%1'.")
			       .arg(attribname);
		uiStringSet messages( errmsg );
		uiMSG().errorWithDetails( messages, msg );
	    }
	    return false;
	}
    }

    return true;
}


bool uiAttribDescSetEd::doCommit( bool useprev )
{
    Desc* usedesc = useprev ? prevdesc_ : curDesc();
    if ( !usedesc )
	return false;

    BufferString newattr = uiAF().attrNameOf( attrtypefld_->attr() );
    BufferString oldattr = usedesc->attribName();
    bool checkusrref = true;
    if ( oldattr != newattr )
    {
       uiString msg = tr("This will change the type of "
			 " existing attribute '%1'.\n"
			 "This will remove previous"
			 " definition of the attribute.\n"
			 "If you want to avoid this please use"
			 " 'Cancel' and 'Add as new'."
			 "\nAre you sure you want"
			 " to change the attribute type?")
			.arg(usedesc->userRef());

	bool res = uiMSG().askGoOn(msg, tr("Change"),
				   uiStrings::sCancel());
	if ( res )
	{
	    checkusrref = false;
	    DescID id = usedesc->id();
	    TypeSet<DescID> attribids;
	    attrset_->getIds( attribids );
	    int oldattridx = attribids.indexOf( id );
	    Desc* newdesc = createAttribDesc( checkusrref );
	    if ( !newdesc ) return false;

	    attrset_->removeDesc( id );
	    if ( !attrset_->errMsg().isEmpty() )
	    {
		uiMSG().error( attrset_->errMsg() );
		newdesc->unRef();
		return false;
	    }
	    attrset_->insertDesc( newdesc, oldattridx, id );
	    const int curidx = attrdescs_.indexOf( curDesc() );
	    newList( curidx );
	    removeNotUsedAttr();
	    adsman_->setSaved( false );
	    usedesc = newdesc;
	}
	else
	{
	    updateFields();
	    return false;
	}
    }

    if ( checkusrref && !setUserRef( usedesc ) )
	return false;

    uiAttrDescEd* curdesced = curDescEd();
    if ( !curdesced )
	return false;


    uiString res = curdesced->commit();
    if ( !res.isEmpty() )
	mErrRetFalse( res )

    return true;
}


void uiAttribDescSetEd::updateUserRefs()
{
    BufferString selnm( attrlistfld_ ? attrlistfld_->getText() : "" );
    userattrnames_.erase();
    attrdescs_.erase();

    for ( int iattr=0; iattr<attrset_->size(); iattr++ )
    {
	const DescID descid = attrset_->getID( iattr );
	Desc* desc = attrset_->getDesc( descid );
	if ( !desc || desc->isHidden() || desc->isStored() ) continue;

	attrdescs_ += desc;
	userattrnames_.add( desc->userRef() );
    }
}


Desc* uiAttribDescSetEd::curDesc() const
{
    int selidx = attrlistfld_->currentItem();
    uiAttribDescSetEd* nct = const_cast<uiAttribDescSetEd*>( this );
    return selidx < 0 ? 0 : nct->attrdescs_[selidx];
}


uiAttrDescEd* uiAttribDescSetEd::curDescEd()
{
    const char* attrstr = attrtypefld_->attr();
    BufferString attrnm = uiAF().attrNameOf( attrstr );
    if ( attrnm.isEmpty() )
	attrnm = attrstr;

    uiAttrDescEd* ret = 0;
    for ( int idx=0; idx<desceds_.size(); idx++ )
    {
	uiAttrDescEd* de = desceds_[idx];
	if ( !de )
	    continue;

	if ( attrnm == de->attribName() )
	    return de;
    }
    return ret;
}


void uiAttribDescSetEd::updateCurDescEd()
{
    curDescEd()->setDesc( curDesc(), adsman_ );
}


bool uiAttribDescSetEd::validName( const char* newnm ) const
{
    if ( !iswalnum(newnm[0]) )
	mErrRetFalse(tr("Please start attribute name with a letter or number"));

    if ( firstOcc(newnm,'!') || firstOcc(newnm,':') || firstOcc(newnm,';') ||
	 firstOcc(newnm,'#') )
       mErrRetFalse(tr("Attribute name may not contain '!', '#', ';' or ':'."));

    const StringView fsnewnm( newnm );
    if ( fsnewnm.size() < 2 )
	mErrRetFalse( tr("Please enter a name of at least 2 characters.") );

    TypeSet<DescID> ids;
    attrset_->getIds( ids );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const Desc& ad = *attrset_->getDesc( ids[idx] );
	if ( fsnewnm == ad.userRef() )
	{
	    uiMSG().error( tr("The name you entered for the attribute already"
			      " exists.\nPlease choose another name.") );
	    return false;
	}
    }

    return true;
}


bool uiAttribDescSetEd::setUserRef( Desc* attrdesc )
{
    BufferString usertypednm( attrnmfld_->text() );
    char* ptr = usertypednm.getCStr();
    mTrimBlanks( ptr );
    const BufferString newnm( ptr );

    if ( newnm == attrdesc->userRef() ) return true;
    else if ( !validName(newnm) ) return false;

    uiString res = curDescEd()->commit();
    if ( !res.isEmpty() )
	{ uiMSG().error( res ); return false; }

    attrdesc->setUserRef( newnm );
    int selidx = userattrnames_.indexOf( attrlistfld_->getText() );
    newList( selidx );
    return true;
}


void uiAttribDescSetEd::updateAttrName()
{
    Desc* curdesc = curDesc();
    attrnmfld_->setText( curdesc ? curdesc->userRef() : "" );
}


bool uiAttribDescSetEd::doSetIO( bool forread )
{
    if ( !setctio_.ioobj_ )
    {
	if ( setid_.isUdf() )
	    return false;

	setctio_.ioobj_ = IOM().get( setid_ );
	if ( !setctio_.ioobj_ )
	    mErrRetFalse(tr("Cannot find attribute set in data base"))
    }

    uiString bs;
    if ( forread )
    {
	Attrib::DescSet attrset( is2D() );
	if ( !AttribDescSetTranslator::retrieve(attrset,setctio_.ioobj_,bs) )
	    mErrRetFalse(bs)

	if ( attrset.is2D() != is2D() )
	{
	    bs = tr("Attribute Set %1 is of type %2")
	       .arg(setctio_.ioobj_->name())
	       .arg(attrset.is2D() ? uiStrings::s2D()
				   : uiStrings::s3D());
	    mErrRetFalse(bs)
	}

	*attrset_ = attrset;
	adsman_->setDescSet( attrset_ );
	adsman_->fillHist();
    }
    else if ( !AttribDescSetTranslator::store(*attrset_,setctio_.ioobj_,bs) )
	mErrRetFalse(bs)

    if ( !bs.isEmpty() )
	{ pErrMsg( bs.getFullString() ); }

    setid_ = setctio_.ioobj_->key();
    return true;
}


void uiAttribDescSetEd::newSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    adsman_->inputHistory().setEmpty();
    updateFields();

    attrset_->removeAll( true );
    setctio_.ioobj_ = 0;
    setid_.setUdf();
    updateUserRefs();
    newList( -1 );
    attrsetfld_->setText( sKeyNotSaved );
    adsman_->setSaved( true );
}


void uiAttribDescSetEd::openSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    setctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( this, setctio_ );
    if ( dlg.go() && dlg.ioObj() )
	openAttribSet( dlg.ioObj() );

}

void uiAttribDescSetEd::openAttribSet( const IOObj* ioobj )
{
    if ( !ioobj ) return;
    IOObj* oldioobj = setctio_.ioobj_; setctio_.ioobj_ = 0;
    setctio_.setObj( ioobj->clone() );
    if ( !doSetIO( true ) )
	setctio_.setObj( oldioobj );
    else
    {
	delete oldioobj;
	setid_ = setctio_.ioobj_->key();
	if ( attrset_->couldBeUsedInAnyDimension() )
	    replaceStoredAttr();

	newList( -1 );
	attrsetfld_->setText( setctio_.ioobj_->name() );
	adsman_->setSaved( true );
	TypeSet<DescID> ids;
	attrset_->getIds( ids );
	for ( int idx=0; idx<attrset_->size(); idx++ )
	{
	    Desc* ad = attrset_->getDesc( ids[idx] );
	    if ( !ad ) continue;
	    if ( ad->isStored() && ad->isSatisfied()==2 )
	    {
		uiString msg = tr("The attribute: '%1'"
				  "will be removed\n"
				  "Storage ID is no longer valid")
			       .arg(ad->userRef());


		uiMSG().message( msg );
		attrset_->removeDesc( ad->id() );
		idx--;
	    }
	}
    }

    applycb.trigger();
}


void uiAttribDescSetEd::defaultSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    BufferStringSet attribfiles;
    BufferStringSet attribnames;
    getDefaultAttribsets( attribfiles, attribnames );

    uiSelectFromList::Setup sflsu( tr("Default Attribute Sets"), attribnames );
    sflsu.dlgtitle( tr("Select default attribute set") );
    uiSelectFromList dlg( this, sflsu );
    dlg.setHelpKey( mODHelpKey(mAttribDescSetEddefaultSetHelpID) );
    if ( !dlg.go() ) return;

    const int selitm = dlg.selection();
    if ( selitm < 0 ) return;
    const char* filenm = attribfiles[selitm]->buf();

    importFromFile( filenm );
    attrsetfld_->setText( sKeyNotSaved );
}


void uiAttribDescSetEd::loadDefaultAttrSet( const char* attribsetnm )
{
    BufferStringSet attribfiles;
    BufferStringSet attribnames;
    getDefaultAttribsets( attribfiles, attribnames );
    const int selidx = attribnames.indexOf( attribsetnm );
    if ( selidx>=0 )
    {
	const char* filenm = attribfiles[selidx]->buf();
	importFromFile( filenm );
	attrsetfld_->setText( sKeyNotSaved );
    }
}


static void gtDefaultAttribsets( const char* dirnm, bool is2d,
				 BufferStringSet& attribfiles,
				 BufferStringSet& attribnames )
{
    if ( !dirnm || !File::exists(dirnm) )
	return;

    DirList attrdl( dirnm, File::DirsInDir, "*Attribs" );
    for ( int idx=0; idx<attrdl.size(); idx++ )
    {
	FilePath fp( dirnm, attrdl.get(idx), "index" );
	od_istream strm( fp.fullPath() );
	ascistream astream( strm, true );

	if ( atEndOfSection(astream) )
	    astream.next();

	const char* typeprefix = is2d ? "2D" : "3D";
	while ( !atEndOfSection(astream) )
	{
	    const char* attribsetnm = astream.keyWord();
	    if ( !StringView(attribsetnm).startsWith(typeprefix) )
	    {
		astream.next();
		continue;
	    }

	    const BufferString attrsetnm( attribsetnm + 3 );
	    BufferString attrfnm = astream.value();
	    if ( !FilePath(attrfnm).isAbsolute() )
	    {
		fp.setFileName( attrfnm );
		attrfnm = fp.fullPath();
	    }

	    if ( !File::exists(attrfnm) )
	    {
		astream.next();
		continue;
	    }

	    attribnames.add( attrsetnm );
	    attribfiles.add( attrfnm );
	    astream.next();
	}
    }
}


void uiAttribDescSetEd::getDefaultAttribsets( BufferStringSet& attribfiles,
					      BufferStringSet& attribnames )
{
    const bool is2d = adsman_ ? adsman_->is2D() : attrset_->is2D();
    gtDefaultAttribsets( mGetApplSetupDataDir(), is2d, attribfiles,
			 attribnames );
    gtDefaultAttribsets( mGetSWDirDataDir(), is2d, attribfiles, attribnames );
}


void uiAttribDescSetEd::showMatrix( CallBacker* )
{
    const HelpKey key( WebsiteHelp::sKeyFactoryName(),
		       WebsiteHelp::sKeyAttribMatrix() );
    HelpProvider::provideHelp( key );
}


void uiAttribDescSetEd::importFromSeis( CallBacker* )
{
    if ( !offerSetSave() ) return;

    // TODO: Only display files with have saved attributes
    const bool is2d = adsman_ ? adsman_->is2D() : attrset_->is2D();
    IOObjContext ctxt( uiSeisSel::ioContext(is2d?Seis::Line:Seis::Vol,true) );
    ctxt.toselect_.require_.set( sKey::Type(), sKey::Attribute() );

    uiSeisSelDlg dlg( this, ctxt, uiSeisSel::Setup(is2d,false) );
    if ( !dlg.go() )
	return;
    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj )
	return;

    FilePath fp( ioobj->fullUserExpr() );
    fp.setExtension( "proc" );
    if ( !File::exists(fp.fullPath()) )
    {
	uiMSG().error( tr("No attributeset stored with this dataset") );
	return;
    }

    IOPar iopar( "AttributeSet" );
    iopar.read( fp.fullPath(), sKey::Pars(), false );
    PtrMan<IOPar> attrpars = iopar.subselect( sKey::Attributes() );
    if ( !attrpars )
    {
	uiMSG().error( tr("Cannot read attributeset from this dataset") );
	return;
    }

    attrset_->usePar( *attrpars );
    newList( -1 );
    attrsetfld_->setText( sKeyNotSaved );
    setctio_.ioobj_ = 0;
    applycb.trigger();
}


void uiAttribDescSetEd::importFromFile( const char* filenm )
{
    od_istream strm( filenm );
    ascistream ascstrm( strm );
    IOPar iopar( ascstrm );
    replaceStoredAttr( iopar );
    attrset_->usePar( iopar );
    newList( -1 );
    attrsetfld_->setText( sKeyNotSaved );
    setctio_.ioobj_ = 0;
    applycb.trigger();
}


void uiAttribDescSetEd::importSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiSelObjFromOtherSurvey objdlg( this, setctio_ );
    objdlg.setHelpKey( mODHelpKey(mAttribDescSetEdimportSetHelpID) );
    IOObj* oldioobj = setctio_.ioobj_; setctio_.ioobj_ = 0;
    if ( objdlg.go() && setctio_.ioobj_ )
    {
	if ( !doSetIO( true ) )
	    setctio_.setObj( oldioobj );
	else
	{
	    delete oldioobj;
	    setid_ = setctio_.ioobj_->key();
	    objdlg.setDirToCurrentSurvey();
	    replaceStoredAttr();
	    newList( -1 );
	    attrsetfld_->setText( sKeyNotSaved );
	    setctio_.ioobj_ = 0;
	    applycb.trigger();
	}
    }
}


void uiAttribDescSetEd::importFile( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiGetFileForAttrSet dlg( this, true, inoutadsman_->is2D() );
    if ( dlg.go() )
	importFromFile( dlg.fileName() );
}


void uiAttribDescSetEd::job2Set( CallBacker* )
{
    if ( !offerSetSave() ) return;
    uiGetFileForAttrSet dlg( this, false, inoutadsman_->is2D() );
    if ( !dlg.go() ) return;

    if ( dlg.attrSet().nrDescs(false,false) < 1 )
	mErrRet( tr("No usable attributes in file") )

    *attrset_ = dlg.attrSet();
    adsman_->setSaved( false );

    setctio_.setObj( 0 );
    newList( -1 ); attrsetfld_->setText( sKeyNotSaved );
    applycb.trigger();
}


void uiAttribDescSetEd::crossPlot( CallBacker* )
{
    if ( !adsman_ || !adsman_->descSet() ) return;
    xplotcb.trigger();
}


void uiAttribDescSetEd::directShow( CallBacker* )
{
    if ( !curDesc() )
	mErrRet( tr("Please add this attribute first") )

    if ( doCommit() )
	dirshowcb.trigger();
    updateFields();
}


void uiAttribDescSetEd::procAttribute( CallBacker* )
{
    if ( !curDesc() )
	mErrRet( tr("Please add this attribute first") )

    if ( !doCommit() ) return;

    uiAttrVolOut* dlg = new uiAttrVolOut( this, *attrset_, false,
					  0, MultiID::udf() );
    dlg->setInput( curDesc()->id() );
    dlg->show();
}


void uiAttribDescSetEd::evalAttribute( CallBacker* )
{
    if ( !doCommit() ) return;
    evalattrcb.trigger();
}


void uiAttribDescSetEd::crossEvalAttrs( CallBacker* )
{
    if ( !doCommit() ) return;
    crossevalattrcb.trigger();
}


bool uiAttribDescSetEd::offerSetSave()
{
    doCommit( true );
    bool saved = adsman_->isSaved();
    if ( saved ) return true;

    uiString msg = tr( "Attribute set is not saved.\nSave now?" );
    const int res = uiMSG().askSave( msg );
    if ( res==1 )
	return doSave(false);

    return res==0;
}


void uiAttribDescSetEd::changeInput( CallBacker* )
{
    attrlistfld_->setEmpty();
    updateFields();
    replaceStoredAttr();
    newList(-1);
    adsman_->setSaved( false );
    applycb.trigger();
}


void uiAttribDescSetEd::replaceStoredAttr()
{
    uiStoredAttribReplacer replacer( this, attrset_ );
    replacer.go();
}


void uiAttribDescSetEd::replaceStoredAttr( IOPar& iopar )
{
    uiStoredAttribReplacer replacer( this, &iopar, attrset_->is2D() );
    replacer.go();
}


void uiAttribDescSetEd::removeNotUsedAttr()
{
     if ( attrset_ ) attrset_->removeUnused( true );
}


bool uiAttribDescSetEd::is2D() const
{
    if ( adsman_ )
	return adsman_->is2D();
    else if ( attrset_ )
	return attrset_->is2D();
    else
	return false;
}


static const char* sKeyDotPath()	{ return "Dot path"; }
static const char* sUnixDot()		{ return "/usr/bin/dot"; }
static const char* sWinDot()
{ return "C:\\Program Files\\Graphviz\\bin\\dot.exe"; }

class uiWhereIsDotDlg : public uiDialog
{ mODTextTranslationClass(uiWhereIsDotDlg)
public:
uiWhereIsDotDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Graphviz/Dot "),mNoDlgTitle,
			mODHelpKey(mWhereIsDotDlgHelpID)))
{
    uiString txt = tr("To display the attribute graph an installation of "
	"Graphviz is required.\nGraphviz can be downloaded from: "
	"http://www.graphviz.org\n\n"
	"Once installed, please select here '%1' from your Graphviz "
	"installation.\ne.g.: %2\n")
	.arg(__iswin__?"dot.exe":"dot")
	.arg(__iswin__?sWinDot():sUnixDot());
    uiLabel* lbl = new uiLabel( this, txt );

    BufferString pathfromsetts;
    Settings::common().get( sKeyDotPath(), pathfromsetts );

    dotfld_ = new uiFileInput( this, tr("Dot executable"),
			       pathfromsetts.buf() );
    dotfld_->attach( leftAlignedBelow, lbl );
}


const char* fileName() const
{ return dotfld_->fileName(); }


bool acceptOK( CallBacker* ) override
{
    const BufferString fnm = fileName();
    if ( File::exists(fnm) && File::isExecutable(fnm) )
    {
	const FilePath fp( fnm );
	if ( fp.baseName() != "dot" )
	{
	    const bool res = uiMSG().askGoOn( tr("It looks like you did not "
		" select the dot executable.\n\nDo you want to continue?") );
	    if ( !res )
		return false;
	}

	Settings::common().set( sKeyDotPath(), fnm.buf() );
	Settings::common().write();
	return true;
    }

    uiMSG().error( tr("Selected file does not exist or is not executable") );
    return false;
}

    uiFileInput*	dotfld_;
};


static bool initDotPath( BufferString& dotpath )
{
    Settings::common().get( sKeyDotPath(), dotpath );
    if ( File::exists(dotpath) )
	return true;

    dotpath = __iswin__ ? sWinDot() : sUnixDot();
    if ( !File::exists(dotpath) )
	return false;

    Settings::common().set( sKeyDotPath(), dotpath.buf() );
    return Settings::common().write();
}


void uiAttribDescSetEd::dotPathCB( CallBacker* )
{
    BufferString dotpath;
    initDotPath( dotpath );
    uiWhereIsDotDlg dlg( this );
    dlg.go();
}


void uiAttribDescSetEd::exportToDotCB( CallBacker* )
{
    if ( !attrset_ || attrlistfld_->isEmpty() )
	return;

    BufferString dotpath;
    if ( !initDotPath(dotpath) )
    {
	uiWhereIsDotDlg dlg( this );
	if ( !dlg.go() )
	    return;

	dotpath = dlg.fileName();
    }

    const BufferString fnm = FilePath::getTempFullPath( "attrs", "dot" );
    const char* attrnm = IOM().nameOf( setid_ );
    attrset_->exportToDot( attrnm, fnm );

    FilePath outputfp( fnm );
    outputfp.setExtension( "png" );
    OS::MachineCommand machcomm( dotpath );
    machcomm.addKeyedArg( "Tpng", fnm, OS::OldStyle );
    machcomm.addKeyedArg( "o", outputfp.fullPath(), OS::OldStyle );
    OS::CommandLauncher cl( machcomm );
    if ( !cl.execute() )
	{ uiMSG().error( cl.errorMsg() ); return; }

    uiDesktopServices::openUrl( outputfp.fullPath() );
}


void uiAttribDescSetEd::updtAllEntries()
{
    PF().updateAllDescsDefaults();
}


bool uiAttribDescSetEd::getUiAttribParamGrps( uiParent* uip,
	ObjectSet<AttribParamGroup>& res, BufferStringSet& paramnms,
	TypeSet<BufferStringSet>& usernms )
{
    if ( !curDesc() )
	return false;

    const bool is2d = inoutadsman_ ? inoutadsman_->is2D() : false;

    TypeSet<Attrib::DescID> adids;
    curDesc()->getDependencies( adids );
    adids.insert( 0, curDesc()->id() );

    ObjectSet<uiAttrDescEd> desceds2erase;
    ObjectSet<uiAttrDescEd> desceds;
    TypeSet<EvalParam> eps;

    for ( int idx=0; idx<adids.size(); idx++ )
    {
	Attrib::Desc* ad = attrset_->getDesc( adids[idx] );
	if ( !ad )
	    continue;

	const char* usernm = ad->userRef();
	const BufferString attrnm = ad->attribName();
	uiAttrDescEd* de = uiAF().create( nullptr, attrnm.buf(), is2d, false );
	if ( !de )
	    continue;

	desceds2erase += de;
	de->setDesc( ad, nullptr );
	TypeSet<EvalParam> tmp;
	de->getEvalParams( tmp );
	for ( int idz=0; idz<tmp.size(); idz++ )
	{
	    const int pidx = eps.indexOf(tmp[idz]);
	    if ( pidx>=0 )
		usernms[pidx].add( usernm );
	    else
	    {
		eps += tmp[idz];
		paramnms.add( tmp[idz].label_ );
		usernms += BufferStringSet(usernm);
		desceds += de;
	    }
	}
    }

    for ( int idx=0; idx<eps.size(); idx++ )
	res += new AttribParamGroup( uip, *desceds[idx], eps[idx] );

    deepErase( desceds2erase );
    // Assumes AttribParamGroup doesn't need it anymore
    return eps.size();
}
