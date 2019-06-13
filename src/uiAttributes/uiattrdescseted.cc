/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "uiattrdescseted.h"

#include "ascstream.h"
#include "attribfactory.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pickset.h"
#include "plugins.h"
#include "ptrman.h"
#include "seistype.h"
#include "survinfo.h"
#include "settings.h"

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
#include "uifilesel.h"
#include "uigeninput.h"
#include "uihelpview.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uisurvioobjseldlg.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uistoredattrreplacer.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

static uiString	lastusedgroup_;
static bool	lastsavebuttonchecked_ = true;


uiAttribDescSetEd::uiAttribDescSetEd( uiParent* p, DescSet& ds,
				      uiString prefgrp, bool attrsneedupdt )
    : uiDialog(p,uiDialog::Setup( ds.is2D() ? tr("Attribute Set 2D")
					: tr("Attribute Set 3D"),mNoDlgTitle,
					mODHelpKey(mAttribDescSetEdHelpID) )
	.savebutton(true).savetext(tr("Save on Close"))
	.menubar(true).modal(false))
    , userattrnames_(*new BufferStringSet)
    , prevdesc_(0)
    , dirshowcb(this)
    , evalattrcb(this)
    , crossevalattrcb(this)
    , xplotcb(this)
    , applycb(this)
    , attrset_(ds)
    , orgattrset_(new DescSet(ds))
    , updating_fields_(false)
    , attrsneedupdt_(attrsneedupdt)
    , zdomaininfo_(0)
{
    setOkCancelText( uiStrings::sClose(), uiString::empty() );

    createMenuBar();
    createToolBar();
    createGroups();

    if ( !prefgrp.isEmpty() )
	lastusedgroup_ = prefgrp;
    attrtypefld_->setGroupName( lastusedgroup_ );

    init();
}


uiAttribDescSetEd::~uiAttribDescSetEd()
{
    delete &userattrnames_;
    delete orgattrset_;
}


bool uiAttribDescSetEd::is2D() const
{
    return attrset_.is2D();
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

#define mInsertItem( txt, func, fnm ) mInsertMnuItem(filemnu,txt,func,fnm)

void uiAttribDescSetEd::createMenuBar()
{
    uiMenuBar* menubar = menuBar();
    if( !menubar )
	{ pErrMsg("huh?"); return; }

    uiMenu* filemnu = new uiMenu( this, uiStrings::sFile() );
    mInsertItem( m3Dots(tr("Open set")), openSetCB, "open" );
    mInsertItem( m3Dots(tr("Save set")), savePushCB, "save" );
    mInsertItem( m3Dots(tr("Save set as")), saveAsPushCB, "saveas" );
    mInsertItem( m3Dots(tr("Clear set")), newSetCB, "clear" );
    mInsertItem( m3Dots(tr("Auto Load Attribute Set")), autoAttrSetCB, "auto" );
    mInsertItem( m3Dots(tr("Change attribute input(s)")),
				    chgAttrInputsCB, "inputs" );
    filemnu->insertSeparator();
    mInsertItem( m3Dots(tr("Open Default set")), openDefSetCB, "defset" );
    uiMenu* impmnu = new uiMenu( this, uiStrings::sImport() );
    impmnu->setIcon( "import" );
    mInsertMnuItem( impmnu, m3Dots(tr("From other Survey")),
		    importSetCB, "survey" );
    mInsertMnuItem( impmnu, m3Dots(tr("From File")), importFileCB,
			"singlefile" );
    uiMenu* recmnu = new uiMenu( this, tr("Reconstruct") );
    recmnu->setIcon( "reconstruct" );
    mInsertMnuItem( recmnu, m3Dots(tr("From Seismics")), importFromSeisCB,
		    "seis" );
    mInsertMnuItem( recmnu, m3Dots(tr("From job file")), job2SetCB, "job2set");

    filemnu->addMenu( impmnu );
    filemnu->addMenu( recmnu );
    menubar->addMenu( filemnu );
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, tip, mCB(this,uiAttribDescSetEd,func) )

void uiAttribDescSetEd::createToolBar()
{
    toolbar_ = new uiToolBar( this, tr("AttributeSet tools") );
    mAddButton( "open", openSetCB, tr("Open attribute set") );
    mAddButton( "defset", openDefSetCB, tr("Open default attribute set") );
    mAddButton( "import", importSetCB,
		tr("Import attribute set from other survey") );
    mAddButton( "job2set", job2SetCB, tr("Reconstruct set from job file") );
    mAddButton( "save", savePushCB, tr("Save attribute set") );
    mAddButton( "saveas", saveAsPushCB, tr("Save attribute set as") );
    mAddButton( "clear", newSetCB, tr("Clear attributes") );
    toolbar_->addSeparator();
    mAddButton( "evalattr", evalAttributeCB, tr("Evaluate attribute") );
    mAddButton( "evalcrossattr",crossEvalAttrsCB,
		tr("Cross attributes evaluate"));
    mAddButton( "xplot", crossPlotCB, tr("Cross-Plot attributes") );
    const int dotidx = mAddButton( "dot", exportToGraphVizDotCB,
			    tr("View as graph") );
    uiMenu* mnu = toolbar_->addButtonMenu( dotidx );
    mnu->insertAction( new uiAction(tr("Graphviz Installation"),
	mCB(this,uiAttribDescSetEd,graphVizDotPathCB)) );
}


void uiAttribDescSetEd::createGroups()
{
    //  Left part
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    stornmfld_ = new uiGenInput(leftgrp, tr("Attribute set"), StringInpSpec());
    stornmfld_->setReadOnly( true );

    attrlistfld_ = new uiListBox( leftgrp, "Defined Attributes" );
    attrlistfld_->setStretch( 2, 2 );
    attrlistfld_->attach( leftAlignedBelow, stornmfld_ );
    mAttachCB( attrlistfld_->selectionChanged, uiAttribDescSetEd::selChgCB );

    moveupbut_ = new uiToolButton( leftgrp, uiToolButton::UpArrow,
				   uiStrings::sUp(),
				   mCB(this,uiAttribDescSetEd,moveUpDownCB) );
    moveupbut_->attach( centeredRightOf, attrlistfld_ );
    movedownbut_ = new uiToolButton( leftgrp, uiToolButton::DownArrow,
				     uiStrings::sDown(),
				     mCB(this,uiAttribDescSetEd,moveUpDownCB) );
    movedownbut_->attach( alignedBelow, moveupbut_ );
    sortbut_ = new uiToolButton( leftgrp, "sort", tr("Sort attributes"),
				 mCB(this,uiAttribDescSetEd,sortPushCB) );
    sortbut_->attach( alignedBelow, movedownbut_ );
    rmbut_ = new uiToolButton( leftgrp, "remove", tr("Remove selected"),
				mCB(this,uiAttribDescSetEd,rmPushCB) );
    rmbut_->attach( alignedBelow, sortbut_ );

    //  Right part
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );

    uiGroup* degrp = new uiGroup( rightgrp, "DescEdGroup" );
    degrp->setStretch( 1, 1 );

    attrtypefld_ = new uiAttrTypeSel( degrp );
    for ( int idx=0; idx<uiAF().size(); idx++ )
    {
	uiAttrDescEd::DomainType dt =
		(uiAttrDescEd::DomainType)uiAF().domainType( idx );
	if ( (dt == uiAttrDescEd::Depth && SI().zIsTime())
		|| (dt == uiAttrDescEd::Time && !SI().zIsTime()) )
	    continue;

	const bool is2d = attrset_.is2D();
	uiAttrDescEd::DimensionType dimtyp =
		(uiAttrDescEd::DimensionType)uiAF().dimensionType( idx );
	if ( (dimtyp == uiAttrDescEd::Only3D && is2d)
		|| (dimtyp == uiAttrDescEd::Only2D && !is2d) )
	    continue;

	const uiString attrnm = uiAF().getDisplayName(idx);
	uiAttrDescEd* de = uiAF().create( degrp, attrnm, is2d );
	if ( !de )
	    continue;

	attrtypefld_->add( uiAF().getGroupName(idx), attrnm );
	if ( zdomaininfo_ )
	    de->setZDomainInfo( zdomaininfo_ );

	de->setInitialDefaults( attrset_ );
	desceds_ += de;
	de->attach( alignedBelow, attrtypefld_ );
    }
    attrtypefld_->update();
    mAttachCB( attrtypefld_->selChg, uiAttribDescSetEd::attrTypSelCB );
    degrp->setHAlignObj( attrtypefld_ );

    helpbut_ = new uiToolButton( degrp, "contexthelp", uiStrings::sHelp(),
				mCB(this,uiAttribDescSetEd,helpButPushCB) );
    helpbut_->attach( rightTo, attrtypefld_ );
    uiToolButton* matrixbut = new uiToolButton( degrp, "attributematrix",
	tr("Documentation: Show Attribute 'Matrix'"),
	mCB(this,uiAttribDescSetEd,showMatrixCB) );
    matrixbut->attach( rightTo, helpbut_ );

    attrnmfld_ = new uiGenInput( rightgrp, uiStrings::sAttribName() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, degrp );
    attrnmfld_->updateRequested.notify( mCB(this,uiAttribDescSetEd,addPushCB) );

    addbut_ = new uiPushButton( rightgrp, tr("Add as new"), true );
    addbut_->attach( rightTo, attrnmfld_ );
    addbut_->setIcon( "addnew" );
    addbut_->activated.notify( mCB(this,uiAttribDescSetEd,addPushCB) );

    dispbut_ = new uiToolButton( rightgrp, "showattrnow",
	tr("Recalculate this attribute on selected element"),
	mCB(this,uiAttribDescSetEd,directShowCB) );
    dispbut_->attach( rightTo, addbut_ );

    procbut_ = new uiToolButton( rightgrp, "out_seis",
				tr("Process this attribute"),
				mCB(this,uiAttribDescSetEd,procAttributeCB) );
    procbut_->attach( rightTo, dispbut_ );

    uiSplitter* splitter = new uiSplitter( this );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


void uiAttribDescSetEd::init()
{
    newList(0);

    setSaveButtonChecked( lastsavebuttonchecked_ );
    setButStates();
}


void uiAttribDescSetEd::setSensitive( bool yn )
{
    topGroup()->setSensitive( yn );
    menuBar()->setSensitive( yn );
    toolbar_->setSensitive( yn );
}


DBKey uiAttribDescSetEd::curSetID() const
{
    return attrset_.storeID();
}



#define mErrRetFalse(s) { uiMSG().error( s ); return false; }
#define mErrRet(s) { uiMSG().error( s ); return; }
#define mErrRetNull(s) { uiMSG().error( s ); return 0; }


void uiAttribDescSetEd::attrTypSelCB( CallBacker* )
{
    updateFields( false );
}


void uiAttribDescSetEd::selChgCB( CallBacker* )
{
    if ( updating_fields_ )
	return;

    doCommit( true );
    updateFields();
    prevdesc_ = curDesc();
    setButStates();
    applycb.trigger();
}


void uiAttribDescSetEd::savePushCB( CallBacker* )
{
    doSave( false );
}


void uiAttribDescSetEd::saveAsPushCB( CallBacker* )
{
    doSave( true );
}


bool uiAttribDescSetEd::doSave( bool issaveas )
{
    if ( !doCommit() )
	return false;

    uiRetVal uirv;
    PtrMan<CtxtIOObj> ctio = attrset_.getCtxtIOObj( false );
    if ( !issaveas && ctio->ioobj_ )
	uirv = attrset_.save();
    else
    {
	uiIOObjSelDlg dlg( this, *ctio );
	if ( !dlg.go() || !dlg.ioObj() )
	    return false;
	uirv = attrset_.store( dlg.ioObj()->key() );
    }

    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return false; }

    setStorNameFld();
    attrset_.setIsChanged( false );
    return true;
}


void uiAttribDescSetEd::setStorNameFld()
{
    const BufferString nm( attrset_.name() );
    stornmfld_->setText( nm.isEmpty() ? tr("<not saved>") : toUiString(nm) );
}


void uiAttribDescSetEd::autoAttrSetCB( CallBacker* )
{
    const bool is2d = is2D();
    uiAutoAttrSelDlg dlg( this, is2d );
    if ( dlg.go() )
    {
	const bool douse = dlg.useAuto();
	IOObj* ioobj = dlg.getObj();
	const DBKey id = ioobj ? ioobj->key() : DBKey::getInvalid();
	Settings::common().setYN( DescSet::sKeyUseAutoAttrSet, douse );
	Settings::common().write();
	IOPar par = SI().getDefaultPars();
	par.set( is2d ? DescSet::sKeyAuto2DAttrSetID
		      : DescSet::sKeyAuto3DAttrSetID, id );
	SI().setDefaultPars( par, true );
	if ( douse && offerSetSave() )
	    openAttribSet( ioobj->key() );
    }
}


void uiAttribDescSetEd::addPushCB( CallBacker* )
{
    Desc* newdesc = createAttribDesc();
    if ( !newdesc )
	return;

    attrset_.addDesc( newdesc );
    newList( attrdescs_.size() );
    attrset_.setIsChanged( true );
    applycb.trigger();
}


Attrib::Desc* uiAttribDescSetEd::createAttribDesc( bool checkuserref )
{
    uiAttrDescEd& curde = activeDescEd();
    BufferString attribname = getAttribName( curde );
    Desc* newdesc = Attrib::PF().createDescCopy( attribname );
    if ( !newdesc )
	mErrRetNull( tr("Internal: cannot create attribute of type '%1'")
		     .arg(attribname) )

    newdesc->ref();
    newdesc->setDescSet( &attrset_ );
    BufferString newnm( attrnmfld_->text() );
    if ( checkuserref )
	ensureValidName( newnm );
    newdesc->setUserRef( newnm );
    uiString res = curde.commit( newdesc );
    if ( !res.isEmpty() )
	{ newdesc->unRef(); mErrRetNull( res ); }

    return newdesc;
}


void uiAttribDescSetEd::helpButPushCB( CallBacker* )
{
    HelpProvider::provideHelp( activeDescEd().helpKey() );
}


void uiAttribDescSetEd::rmPushCB( CallBacker* )
{
    Desc* curdesc = curDesc();
    if ( !curdesc ) return;

    BufferString depattribnm;
    if ( attrset_.isAttribUsed( curdesc->id(), depattribnm ) )
    {
	uiMSG().error( tr("Cannot remove this attribute. It is used\n"
			  "as input for another attribute called '%1'")
			.arg(depattribnm.buf()) );
	return;
    }

    const int curidx = attrdescs_.indexOf( curdesc );
    attrset_.removeDesc( attrset_.getID(*curdesc) );
    newList( curidx );
    attrset_.setIsChanged( true );
    setButStates();
}


void uiAttribDescSetEd::moveUpDownCB( CallBacker* cb )
{
    Desc* curdesc = curDesc();
    if ( !curdesc ) return;

    const bool moveup = cb == moveupbut_;
    const int curidx = attrdescs_.indexOf( curdesc );
    attrset_.moveDescUpDown( attrset_.getID(*curdesc), moveup );
    newList( curidx );
    attrlistfld_->setCurrentItem( moveup ? curidx-1 : curidx+1 );
    attrset_.setIsChanged( true );
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


void uiAttribDescSetEd::sortPushCB( CallBacker* )
{
    attrset_.sortDescSet();
    newList( 0 );
}


void uiAttribDescSetEd::handleSensitivity()
{
    bool havedescs = !attrdescs_.isEmpty();
    rmbut_->setSensitive( havedescs );
}


bool uiAttribDescSetEd::acceptOK()
{
    if ( !curDesc() )
	return true;

    if ( !doCommit() || !doAcceptInputs() )
	return false;

    lastsavebuttonchecked_ = saveButtonChecked();
    if ( lastsavebuttonchecked_ && !doSave(false) )
	return false;

    lastusedgroup_ = attrtypefld_->groupName();
    applycb.trigger();
    return true;
}


bool uiAttribDescSetEd::rejectOK()
{
    if ( attrset_.size() != orgattrset_->size()
      && !uiMSG().askGoOn(tr("Roll back to attribute definitions at entry?")) )
	return false;

    attrset_ = *orgattrset_;
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
    if ( newcur < 0 )
	newcur = 0;
    if ( newcur >= attrlistfld_->size() )
	newcur = attrlistfld_->size()-1;
    if ( !userattrnames_.isEmpty() )
    {
	attrlistfld_->setCurrentItem( newcur );
	prevdesc_ = curDesc();
    }

    updateFields();
    handleSensitivity();
    setButStates();
}


void uiAttribDescSetEd::setSelAttr( const char* attrnm, bool isnewset )
{
    if ( !attrtypefld_ )
	return;

    if ( isnewset )
	newSetCB(0);

    attrtypefld_->setAttributeName( attrnm );
    updateFields( false );
}


BufferString uiAttribDescSetEd::getAttribName( uiAttrDescEd& desced ) const
{
    BufferString attribname = desced.attribName();
    if ( attribname.isEmpty() )
    {
	pErrMsg("Missing uiAttrDescEd attribName()");
	attribname = attrtypefld_->attributeName();
    }
    return attribname;
}


void uiAttribDescSetEd::updateFields( bool set_type )
{
    updating_fields_ = true;

    Desc* curdesc = curDesc();
    attrnmfld_->setText( curdesc ? curdesc->userRef() : "" );

    if ( set_type )
    {
	BufferString attrnm( "RefTime" );
	if ( curdesc )
	    attrnm.set( curdesc->attribName() );
	attrtypefld_->setAttributeName( attrnm );
    }

    uiAttrDescEd& neededdesced = activeDescEd();
    const bool isappropriatedesc = !curdesc
		|| getAttribName( neededdesced ) == curdesc->attribName();
    if ( !isappropriatedesc )
	{ pErrMsg("Remove this msg if this is something legal"); }

    for ( int idx=0; idx<desceds_.size(); idx++ )
    {
	uiAttrDescEd& de = *desceds_[idx];
	const bool istargetdesced = &de == &neededdesced;

	if ( !istargetdesced )
	    de.setDescSet( &attrset_ );
	else if ( isappropriatedesc )
	    de.setDesc( curdesc );

	de.display( istargetdesced );
    }

    updating_fields_ = false;
}


bool uiAttribDescSetEd::doAcceptInputs()
{
    uiAttrDescEd& curdesced = activeDescEd();
    for ( int idx=0; idx<attrset_.size(); idx++ )
    {
	const Attrib::DescID descid = attrset_.getID( idx );
	Desc* desc = attrset_.getDesc( descid );
	uiRetVal uirv = curdesced.errMsgs( desc );
	if ( !uirv.isOK() )
	{
	    if ( desc->isStored() )
		uirv.insert( uiStrings::phrErrDuringRead(desc->userRef()) );
	    else
		uirv.insert( tr("'%1' has incorrect input(s)")
				.arg(desc->userRef()) );
	    uiMSG().error( uirv );
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

    BufferString newattr = attrtypefld_->attributeName();
    BufferString oldattr = usedesc->attribName();
    if ( oldattr != newattr )
    {
       uiString msg = tr("This will change the type of "
			 " existing attribute '%1'.\n"
			 "It will remove the previous"
			 " definition of the attribute.\n"
			 "If you want to avoid this please use"
			 " 'Cancel' and 'Add as new'."
			 "\n\nAre you sure you want"
			 " to change the attribute type?")
			.arg(usedesc->userRef());

	bool chg_type = uiMSG().askGoOn(msg, tr("Change Type"),
				   uiStrings::sCancel());
	if ( chg_type )
	{
	    Attrib::DescID id = usedesc->id();
	    TypeSet<Attrib::DescID> attribids;
	    attrset_.getIds( attribids );
	    int oldattridx = attribids.indexOf( id );
	    Desc* newdesc = createAttribDesc( false );
	    if ( !newdesc )
		return false;

	    attrset_.removeDesc( id );
	    attrset_.insertDesc( newdesc, oldattridx, id );
	    const int curidx = attrdescs_.indexOf( curDesc() );
	    newList( curidx );
	    attrset_.setIsChanged( true );
	    usedesc = newdesc;
	}
	else
	{
	    updateFields();
	    return false;
	}
    }

    if ( !setUserRef(*usedesc) )
	return false;

    uiString res = activeDescEd().commit();
    if ( !res.isEmpty() )
	mErrRetFalse( res )

    return true;
}


void uiAttribDescSetEd::updateUserRefs()
{
    BufferString selnm( attrlistfld_ ? attrlistfld_->getText() : "" );
    userattrnames_.erase();
    attrdescs_.erase();

    for ( int iattr=0; iattr<attrset_.size(); iattr++ )
    {
	const Attrib::DescID descid = attrset_.getID( iattr );
	Desc* desc = attrset_.getDesc( descid );
	if ( !desc || desc->isHidden() || desc->isStored() ) continue;

	attrdescs_ += desc;
	userattrnames_.add( desc->userRef() );
    }
}


Attrib::Desc* uiAttribDescSetEd::curDesc() const
{
    const int selidx = attrlistfld_->currentItem();
    return selidx<0 ? 0 : const_cast<Desc*>( attrdescs_[selidx] );
}


uiAttrDescEd& uiAttribDescSetEd::activeDescEd()
{
    BufferString attrnm = attrtypefld_->attributeName();
    if ( attrnm.isEmpty() )
	{ pErrMsg("Huh"); return *desceds_[0]; }

    for ( int idx=0; idx<desceds_.size(); idx++ )
    {
	uiAttrDescEd& de = *desceds_[idx];
	if ( attrnm == de.attribName() )
	    return de;
    }

    pErrMsg("Module not inited? May crash now.");
    return *desceds_[0];
}


void uiAttribDescSetEd::setCurDescNr( int idx )
{
    newList( idx );
}


void uiAttribDescSetEd::ensureValidName( BufferString& attrnm ) const
{
    attrnm.trimBlanks();
    int sz = attrnm.size();
    char* ptr = attrnm.getCStr();
    for ( int idx=0; idx<sz; idx++ )
    {
	const char c = ptr[idx];
	if ( iscntrl(c)
	  || c == '!' || c == '#' || c == ';' || c == ':' || c == '`' )
	    ptr[idx] = '_';
    }
    if ( attrnm.size() < 2 || !isalpha(attrnm[0]) )
	attrnm.insertAt( 0, "A_" );

    while ( attrset_.isPresent(attrnm) )
    {
	BufferString oldnm( attrnm );
	char* lptr = oldnm.findLast( '{' );
	char* rptr = oldnm.findLast( '}' );
	if ( !lptr || !rptr )
	    attrnm.add( " {2}" );
	else
	{
	    *lptr = '\0'; *rptr = '\0';
	    lptr++; rptr++;
	    int nr = toInt( lptr );
	    if ( nr < 3 )
		nr = 2;
	    nr++;
	    attrnm.set( oldnm ).add( "{" ).add(nr).add( '}' ).add( rptr );
	}
    }
}


bool uiAttribDescSetEd::setUserRef( Desc& desc )
{
    BufferString newnm( attrnmfld_->text() );
    newnm.trimBlanks();
    if ( newnm == desc.userRef() )
	return true;

    ensureValidName( newnm );
    const uiString res = activeDescEd().commit();
    if ( !res.isEmpty() )
	{ uiMSG().error( res ); return false; }

    desc.setUserRef( newnm );
    int selidx = userattrnames_.indexOf( attrlistfld_->getText() );
    newList( selidx );
    return true;
}


void uiAttribDescSetEd::newSetCB( CallBacker* )
{
    if ( !offerSetSave() )
	return;

    updateFields();

    attrset_.setEmpty();
    attrset_.setStoreID( DBKey() );
    attrset_.ensureDefStoredPresent();
    attrset_.setIsChanged( false );
    updateUserRefs();
    newList( -1 );
    setStorNameFld();
}


void uiAttribDescSetEd::openSetCB( CallBacker* )
{
    if ( !offerSetSave() ) return;
    PtrMan<CtxtIOObj> ctio = attrset_.getCtxtIOObj( true );
    uiIOObjSelDlg dlg( this, *ctio );
    if ( dlg.go() && dlg.ioObj() )
	openAttribSet( dlg.ioObj()->key() );

}

void uiAttribDescSetEd::openAttribSet( const DBKey& ky )
{
    if ( !ky.isValid() )
	return;

    DescSet newset( !SI().has3D() );
    uiRetVal uirv = newset.load( ky );
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return; }

    attrset_ = newset;
    handleFreshSet();
}


void uiAttribDescSetEd::handleFreshSet()
{
    newList( -1 );
    setStorNameFld();
    attrset_.setIsChanged( false );
    TypeSet<Attrib::DescID> ids;
    attrset_.getIds( ids );
    for ( int idx=0; idx<attrset_.size(); idx++ )
    {
	Desc* ad = attrset_.getDesc( ids[idx] );
	if ( !ad )
	    continue;
	if ( ad->isStored() && Desc::isError(ad->satisfyLevel()) )
	{
	    uiString msg = tr("The attribute: '%1'"
			      "will be removed\n"
			      "Storage ID is no longer valid")
			   .arg(ad->userRef());

	    uiMSG().message( msg );
	    attrset_.removeDesc( ad->id() );
	    idx--;
	}
    }

    applycb.trigger();
}


void uiAttribDescSetEd::openDefSetCB( CallBacker* )
{
    if ( !offerSetSave() )
	return;

    BufferStringSet attribfiles;
    BufferStringSet attribsetnames;
    getDefaultAttribsets( attribfiles, attribsetnames );

    uiSelectFromList::Setup sflsu( tr("Default Attribute Sets"),
				    attribsetnames );
    sflsu.dlgtitle( tr("Select default attribute set") );
    uiSelectFromList dlg( this, sflsu );
    dlg.setHelpKey( mODHelpKey(mAttribDescSetEddefaultSetHelpID) );
    if ( !dlg.go() )
	return;

    const int selitm = dlg.selection();
    if ( selitm < 0 )
	return;

    const BufferString filenm = attribfiles.get( selitm );
    importFromFile( filenm );
    setStorNameFld();
}


void uiAttribDescSetEd::loadDefaultAttrSet( const char* attribsetnm )
{
    BufferStringSet attribfiles, attribsetnames;
    getDefaultAttribsets( attribfiles, attribsetnames );
    const int selidx = attribsetnames.indexOf( attribsetnm );
    if ( selidx>=0 )
    {
	importFromFile( attribfiles.get(selidx) );
	setStorNameFld();
    }
}


void uiAttribDescSetEd::gtDefAttrSetsInDir( const char* dirnm,
	BufferStringSet& attribfiles, BufferStringSet& attribsetnames ) const
{
    if ( !dirnm || !File::exists(dirnm) )
	return;

    const bool is2d = attrset_.is2D();
    DirList attrdl( dirnm, File::DirsInDir, "*Attribs" );
    for ( int idx=0; idx<attrdl.size(); idx++ )
    {
	File::Path fp( dirnm, attrdl.get(idx), "index" );
	IOPar iopar( "AttributeSet Table" );
	iopar.read( fp.fullPath(), sKey::Pars(), false );
	PtrMan<IOPar> subpar = iopar.subselect( is2d ? "2D" : "3D" );
	if ( !subpar )
	    continue;

	for ( int idy=0; idy<subpar->size(); idy++ )
	{
	    BufferString attrfnm = subpar->getValue( idy );
	    if ( !File::Path(attrfnm).isAbsolute() )
	    {
		fp.setFileName( attrfnm );
		attrfnm = fp.fullPath();
	    }

	    if ( !File::exists(attrfnm) )
		continue;

	    DescSet tmpds( is2d );
	    uiRetVal uirv = tmpds.load( attrfnm );
	    if ( !uirv.isOK() )
		continue;

	    bool canuse = true;
	    for ( int idd=0; idd<tmpds.nrDescs(true,true); idd++ )
	    {
		Desc* desc = tmpds.desc( idd );
		if ( !desc )
		    { canuse = false; break; }

		auto* prov = Attrib::PF().create( *desc, uirv, true );
		canuse = canuse && prov && prov->isActive().isOK();
		prov->unRef();
		if ( !canuse )
		    break;
	    }

	    if ( canuse )
	    {
		attribsetnames.add( subpar->getKey(idy) );
		attribfiles.add( attrfnm );
	    }
	}
    }
}


void uiAttribDescSetEd::getDefaultAttribsets( BufferStringSet& attribfiles,
				      BufferStringSet& attribsetnames ) const
{
    gtDefAttrSetsInDir( mGetApplSetupDataDir(), attribfiles, attribsetnames );
    gtDefAttrSetsInDir( mGetSWDirDataDir(), attribfiles, attribsetnames );
}


void uiAttribDescSetEd::showMatrixCB( CallBacker* )
{
    const HelpKey key( WebsiteHelp::sKeyFactoryName(),
		       WebsiteHelp::sKeyAttribMatrix() );
    HelpProvider::provideHelp( key );
}


void uiAttribDescSetEd::importFromSeisCB( CallBacker* )
{
    if ( !offerSetSave() )
	return;

	// TODO: Only display files with saved attributes
    const bool is2d = attrset_.is2D();
    IOObjContext ctxt( uiSeisSel::ioContext(is2d?Seis::Line:Seis::Vol,true) );
    ctxt.toselect_.require_.set( sKey::Type(), sKey::Attribute() );

    uiSeisSelDlg dlg( this, ctxt, uiSeisSel::Setup(is2d,false) );
    if ( !dlg.go() )
	return;
    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj )
	return;

    File::Path fp( ioobj->mainFileName() );
    fp.setExtension( sProcFileExtension() );
    if ( !File::exists(fp.fullPath()) )
	{ uiMSG().error( tr("Dataset has no stored Attribute Set") ); return; }

    IOPar iopar( "AttributeSet" );
    iopar.read( fp.fullPath(), sKey::Pars(), false );
    PtrMan<IOPar> attrpars = iopar.subselect( sKey::Attributes() );
    if ( !attrpars )
	{ uiMSG().error(tr("No valid Attribute Set in this dataset")); return; }

    attrset_.usePar( *attrpars );
    newList( -1 );
    setStorNameFld();
    applycb.trigger();
}


void uiAttribDescSetEd::importFromFile( const char* filenm )
{
    uiRetVal warns;
    uiRetVal uirv = attrset_.load( filenm, &warns );
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return; }
    else if ( !warns.isOK() )
	uiMSG().warning( warns );

    replaceStoredAttr();
    newList( -1 );
    setStorNameFld();
    applycb.trigger();
}


void uiAttribDescSetEd::importSetCB( CallBacker* )
{
    if ( !offerSetSave() )
	return;

    PtrMan<IOObjContext> ctxt = DescSet::getIOObjContext(true,is2D());
    uiSurvIOObjSelDlg objsel( this, *ctxt );
    objsel.excludeCurrentSurvey();
    objsel.setHelpKey( mODHelpKey(mAttribDescSetEdimportSetHelpID) );
    if ( !objsel.go() )
	return;

    const BufferString filenm( objsel.mainFileName() );
    DescSet impset( !SI().has3D() );
    uiRetVal warns;
    uiRetVal uirv = impset.load( filenm, &warns );
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return; }
    else if ( !warns.isOK() )
	uiMSG().warning( warns );

    replaceStoredAttr();
    newList( -1 );
    setStorNameFld();
    applycb.trigger();
}


void uiAttribDescSetEd::importFileCB( CallBacker* )
{
    if ( !offerSetSave() )
	return;

    uiGetFileForAttrSet dlg( this, true, attrset_.is2D() );
    if ( dlg.go() )
	importFromFile( dlg.fileName() );
}


void uiAttribDescSetEd::job2SetCB( CallBacker* )
{
    if ( !offerSetSave() )
	return;

    uiGetFileForAttrSet dlg( this, false, attrset_.is2D() );
    if ( !dlg.go() )
	return;

    if ( dlg.attrSet().nrDescs(false,false) < 1 )
	mErrRet( tr("No usable attributes in file") )

    attrset_ = dlg.attrSet();
    attrset_.setIsChanged( true );

    newList( -1 );
    setStorNameFld();
    applycb.trigger();
}


void uiAttribDescSetEd::crossPlotCB( CallBacker* )
{
    xplotcb.trigger();
}


void uiAttribDescSetEd::directShowCB( CallBacker* )
{
    if ( !curDesc() )
	mErrRet( tr("Please add this attribute first") )

    if ( doCommit() )
	dirshowcb.trigger();

    updateFields();
}


void uiAttribDescSetEd::procAttributeCB( CallBacker* )
{
    if ( !curDesc() )
	mErrRet( tr("Please add this attribute first") )

    if ( !doCommit() )
	return;

    uiAttrVolOut* dlg = new uiAttrVolOut( this, attrset_, false,
					  0, DBKey::getInvalid() );
    dlg->setInput( curDesc()->id() );
    dlg->show();
}


void uiAttribDescSetEd::evalAttributeCB( CallBacker* )
{
    if ( doCommit() )
	evalattrcb.trigger();
}


void uiAttribDescSetEd::crossEvalAttrsCB( CallBacker* )
{
    if ( doCommit() )
	crossevalattrcb.trigger();
}


bool uiAttribDescSetEd::offerSetSave()
{
    doCommit( true );

    if ( attrset_.isChanged()
      && uiMSG().askSave(
	   uiStrings::phrIsNotSavedSaveNow(uiStrings::sAttributeSet()) ) )
	return doSave( true );

    return true;
}


void uiAttribDescSetEd::chgAttrInputsCB( CallBacker* )
{
    attrlistfld_->setEmpty();
    updateFields();
    replaceStoredAttr();
    newList(-1);
    attrset_.setIsChanged( true );
    applycb.trigger();
}


void uiAttribDescSetEd::replaceStoredAttr()
{
    uiStoredAttribReplacer replacer( this, &attrset_ );
    replacer.go();
}


class uiWhereIsDotDlg : public uiDialog
{ mODTextTranslationClass(uiWhereIsDotDlg)
public:
uiWhereIsDotDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Graphviz/Dot "),mNoDlgTitle,
		 mODHelpKey(mWhereIsDotDlgHelpID)))
{
    uiString txt = tr("To display the attribute graph an installation of \n"
	"Graphviz is required. Graphviz can be downloaded from:\n%1")
	.arg("http://www.graphviz.org");
    uiLabel* lbl = new uiLabel( this, txt );

    BufferString pathfromsetts;
    Settings::common().get( "Dot path", pathfromsetts );

    dotfld_ = new uiFileSel( this, tr("Dot executable"),
			       pathfromsetts.buf() );
    dotfld_->attach( leftAlignedBelow, lbl );
}


const char* fileName() const
{
    return dotfld_->fileName();
}


bool acceptOK()
{
    const BufferString fnm = fileName();
    if ( File::exists(fnm) && File::isExecutable(fnm) )
    {
	const File::Path fp( fnm );
	if ( fp.baseName() != "dot" )
	{
	    const bool res = uiMSG().askGoOn( tr("It looks like you did not "
		" select the dot executable.\n\nDo you want to continue?") );
	    if ( !res ) return false;
	}

	Settings::common().set( "Dot path", fnm.buf() );
	Settings::common().write();
	return true;
    }

    uiMSG().error( tr("Selected file does not exist or is not executable") );
    return false;
}

    uiFileSel*	dotfld_;

};


void uiAttribDescSetEd::graphVizDotPathCB( CallBacker* )
{
    uiWhereIsDotDlg dlg( this );
    dlg.go();
}


void uiAttribDescSetEd::exportToGraphVizDotCB( CallBacker* )
{
    if ( attrlistfld_->isEmpty() ) return;

    BufferString dotpath;
    Settings::common().get( "Dot path", dotpath );
    if ( dotpath.isEmpty() )
    {
	uiWhereIsDotDlg dlg( this );
	if ( !dlg.go() )
	    return;

	dotpath = dlg.fileName();
    }

    const BufferString fnm = File::Path::getTempFullPath( "attrs", "dot" );
    attrset_.exportToDot( name(), fnm );

    File::Path outputfp( fnm );
    outputfp.setExtension( "png" );
    OS::MachineCommand machcomm( dotpath );
    machcomm.addKeyedArg( "Tpng", fnm, OS::OldStyle );
    machcomm.addKeyedArg( "o", outputfp.fullPath(), OS::OldStyle );
    OS::CommandLauncher cl( machcomm );
    if ( !cl.execute() )
	{ uiMSG().error( cl.errorMsg() ); return; }

    uiDesktopServices::openUrl( outputfp.fullPath() );
}


void uiAttribDescSetEd::updateCurDescEd()
{
    curDescEd().setDesc( curDesc() );
}


void uiAttribDescSetEd::updateAllDescsDefaults()
{
    Attrib::PF().updateAllDescsDefaults();
}


bool uiAttribDescSetEd::getUiAttribParamGrps( uiParent* uip,
	ObjectSet<AttribParamGroup>& res, BufferStringSet& paramnms,
	TypeSet<BufferStringSet>& usernms )
{
    if ( !curDesc() )
	return false;

    TypeSet<Attrib::DescID> adids;
    curDesc()->getDependencies( adids );
    adids.insert( 0, curDesc()->id() );

    TypeSet<int> ids;
    TypeSet<EvalParam> eps;

    for ( int idx=0; idx<adids.size(); idx++ )
    {
	const Desc* ad = attrset_.getDesc( adids[idx] );
	if ( !ad )
	    { pErrMsg("Huh"); continue; }

	const BufferString attrnm = ad->attribName();
	const char* usernm = ad->userRef();
	for ( int idy=0; idy<desceds_.size(); idy++ )
	{
	    if ( !desceds_[idy] || attrnm != desceds_[idy]->attribName() )
		continue;

	    TypeSet<EvalParam> tmp;
	    desceds_[idy]->getEvalParams( tmp );
	    for ( int idz=0; idz<tmp.size(); idz++ )
	    {
		const int pidx = eps.indexOf(tmp[idz]);
		if ( pidx>=0 )
		    usernms[pidx].add( usernm );
		else
		{
		    eps += tmp[idz];
		    paramnms.add( tmp[idz].label_ );

		    BufferStringSet unms;
		    unms.add( usernm );
		    usernms += unms;
		    ids += idy;
		}
	    }
	    break;
	}
    }

    for ( int idx=0; idx<eps.size(); idx++ )
	res += new AttribParamGroup( uip, *desceds_[ids[idx]], eps[idx] );

    return eps.size();
}
