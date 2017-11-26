/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
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
#include "dbman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pickset.h"
#include "plugins.h"
#include "ptrman.h"
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
#include "uifilesel.h"
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
    mnu->insertItem( itm ); \
}

#define mInsertMnuItemNoIcon( mnu, txt, func ) \
{ \
    uiAction* itm = new uiAction(txt,mCB(this,uiAttribDescSetEd,func));\
    mnu->insertItem( itm ); \
}

#define mInsertItem( txt, func, fnm ) mInsertMnuItem(filemnu,txt,func,fnm)
#define mInsertItemNoIcon( txt, func ) mInsertMnuItemNoIcon(filemnu,txt,func)

void uiAttribDescSetEd::createMenuBar()
{
    uiMenuBar* menubar = menuBar();
    if( !menubar )
	{ pErrMsg("huh?"); return; }

    uiMenu* filemnu = new uiMenu( this, uiStrings::sFile() );
    mInsertItem( m3Dots(tr("New set")), newSetCB, "new" );
    mInsertItem( m3Dots(tr("Open set")), openSetCB, "open" );
    mInsertItem( m3Dots(tr("Save set")), savePushCB, "save" );
    mInsertItem( m3Dots(tr("Save set as")), saveAsPushCB, "saveas" );
    mInsertItemNoIcon( m3Dots(tr("Auto Load Attribute Set")), autoAttrSetCB );
    mInsertItemNoIcon( m3Dots(tr("Change attribute input(s)")),
				    chgAttrInputsCB );
    filemnu->insertSeparator();
    mInsertItem( m3Dots(tr("Open Default set")), openDefSetCB, "defset" );
    uiMenu* impmnu = new uiMenu( this, uiStrings::sImport() );
    mInsertMnuItem( impmnu, m3Dots(tr("From other Survey")),
		    importSetCB, "impset" );
    mInsertMnuItemNoIcon( impmnu, m3Dots(tr("From File")), importFileCB );
    mInsertItem( m3Dots(tr("Reconstruct from job file")), job2SetCB, "job2set");
    mInsertItemNoIcon( m3Dots(tr("Import set from Seismics")),
			importFromSeisCB );

    filemnu->insertItem( impmnu );
    menubar->insertItem( filemnu );
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, tip, mCB(this,uiAttribDescSetEd,func) )

void uiAttribDescSetEd::createToolBar()
{
    toolbar_ = new uiToolBar( this, tr("AttributeSet tools") );
    mAddButton( "new", newSetCB, tr("New attribute set") );
    mAddButton( "open", openSetCB, tr("Open attribute set") );
    mAddButton( "defset", openDefSetCB, tr("Open default attribute set") );
    mAddButton( "impset", importSetCB,
		tr("Import attribute set from other survey") );
    mAddButton( "job2set", job2SetCB, tr("Reconstruct set from job file") );
    mAddButton( "save", savePushCB, tr("Save attribute set") );
    mAddButton( "saveas", saveAsPushCB, tr("Save attribute set as") );
    toolbar_->addSeparator();
    mAddButton( "evalattr", evalAttributeCB, tr("Evaluate attribute") );
    mAddButton( "evalcrossattr",crossEvalAttrsCB,
		tr("Cross attributes evaluate"));
    mAddButton( "xplot", crossPlotCB, tr("Cross-Plot attributes") );
    const int dotidx = mAddButton( "dot", exportToGraphVizDotCB,
			    tr("View as graph") );
    uiMenu* mnu = new uiMenu(0);
    mnu->insertAction( new uiAction(tr("Graphviz Installation"),
	mCB(this,uiAttribDescSetEd,graphVizDotPathCB)) );
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
    attrlistfld_->attach( leftAlignedBelow, attrsetfld_ );
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

	const bool is2d = inoutadsman_ ? inoutadsman_->is2D() : false;
	uiAttrDescEd::DimensionType dimtyp =
		(uiAttrDescEd::DimensionType)uiAF().dimensionType( idx );
	if ( (dimtyp == uiAttrDescEd::Only3D && is2d)
		|| (dimtyp == uiAttrDescEd::Only2D && !is2d) )
	    continue;

	const char* attrnm = uiAF().getDisplayName(idx);
	attrtypefld_->add( uiAF().getGroupName(idx), attrnm );
	uiAttrDescEd* de = uiAF().create( degrp, attrnm, is2d, true );
	if ( !de )
	    continue;

	if ( zdomaininfo_ )
	    de->setZDomainInfo( zdomaininfo_ );

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
	tr("Attribute Matrix"), mCB(this,uiAttribDescSetEd,showMatrixCB) );
    matrixbut->attach( rightTo, helpbut_ );

    attrnmfld_ = new uiGenInput( rightgrp, uiStrings::sAttribName() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, degrp );
    attrnmfld_->updateRequested.notify( mCB(this,uiAttribDescSetEd,addPushCB) );

    addbut_ = new uiPushButton( rightgrp, tr("Add as new"), true );
    addbut_->attach( rightTo, attrnmfld_ );
    addbut_->setIcon( "plus" );
    addbut_->activated.notify( mCB(this,uiAttribDescSetEd,addPushCB) );

    dispbut_ = new uiToolButton( rightgrp, "showattrnow",
	tr("Recalculate this attribute on selected element"),
	mCB(this,uiAttribDescSetEd,directShowCB) );
    dispbut_->attach( rightTo, addbut_ );

    procbut_ = new uiToolButton( rightgrp, "seisout",
	tr("Process this attribute"),
	mCB(this,uiAttribDescSetEd,procAttributeCB) );
    procbut_->attach( rightTo, dispbut_ );

    uiSplitter* splitter = new uiSplitter( this );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


#define mUnsetAuto SI().removeKeyFromDefaultPars( autoidkey, true );

void uiAttribDescSetEd::init()
{
    delete attrset_;
    attrset_ = new Attrib::DescSet( *inoutadsman_->descSet() );
    delete adsman_;
    adsman_ = new DescSetMan( inoutadsman_->is2D(), attrset_, false );
    adsman_->fillHist();
    adsman_->setSaved( inoutadsman_->isSaved() );

    setid_ = inoutadsman_->attrsetid_;
    setctio_.setObj( DBM().get(setid_) );
    bool autoset = false;
    DBKey autoid;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, autoset );
    const char* autoidkey = is2D() ? uiAttribDescSetEd::sKeyAuto2DAttrSetID
				   : uiAttribDescSetEd::sKeyAuto3DAttrSetID;
    if ( autoset && SI().getDefaultPars().get(autoidkey,autoid) &&
	 autoid != setid_ )
    {
	uiString msg = tr("The Attribute-set selected for Auto-load"
			  " is no longer valid.\n Load another now?");

	if ( uiMSG().askGoOn( msg ) )
	{
	    BufferStringSet attribfiles;
	    BufferStringSet attribnames;
	    BufferStringSet errmsgs;
	    getDefaultAttribsets( attribfiles, attribnames, errmsgs );
	    uiAutoAttrSetOpen dlg( this, attribfiles, attribnames );
	    if ( dlg.go() )
	    {
		if ( dlg.isUserDef() )
		{
		    IOObj* ioobj = dlg.getObj();
		    if ( dlg.isAuto() )
		    {
			if ( ioobj )
			    SI().setDefaultPar( autoidkey,
						ioobj->key().toString(), true );
			else
			    mUnsetAuto
		    }
		    else
			mUnsetAuto

		    openAttribSet( ioobj );
		}
		else
		{
		    const char* filenm = dlg.getAttribfile();
		    const char* attribnm = dlg.getAttribname();
		    const int selidx = attribnames.indexOf( attribnm );
		    if ( !errmsgs.get( selidx ).isEmpty() )
		    {
			uiMSG().error( tr(errmsgs.get( selidx ).buf()) );
			return;
		    }

		    importFromFile( filenm );
		    attrsetfld_->setText( attribnm );
		    mUnsetAuto
		}
	    }
	    else
		mUnsetAuto
	}
	else
	    mUnsetAuto
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
    removeUnusedAttrDescs();
    doSave( true );
}


void uiAttribDescSetEd::saveAsPushCB( CallBacker* )
{
    removeUnusedAttrDescs();
    doSave( false );
}


bool uiAttribDescSetEd::doSave( bool endsave )
{
    if ( !doCommit() )
	return false;

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


void uiAttribDescSetEd::autoAttrSetCB( CallBacker* )
{
    const bool is2d = is2D();
    uiAutoAttrSelDlg dlg( this, is2d );
    if ( dlg.go() )
    {
	const bool douse = dlg.useAuto();
	IOObj* ioobj = dlg.getObj();
	const DBKey id = ioobj ? ioobj->key() : DBKey::getInvalid();
	Settings::common().setYN(uiAttribDescSetEd::sKeyUseAutoAttrSet, douse);
	Settings::common().write();
	IOPar par = SI().getDefaultPars();
	const BufferString idstr = id.toString();
	is2d ? par.set(uiAttribDescSetEd::sKeyAuto2DAttrSetID, idstr.str() )
	     : par.set(uiAttribDescSetEd::sKeyAuto3DAttrSetID, idstr.str() );
	SI().setDefaultPars( par, true );
	if ( dlg.loadAuto() )
	{
	    if ( !offerSetSave() ) return;
	    openAttribSet( ioobj );
	}
    }
}


void uiAttribDescSetEd::addPushCB( CallBacker* )
{
    Desc* newdesc = createAttribDesc();
    if ( !newdesc ) return;
    if ( !attrset_->addDesc(newdesc).isValid() )
	{ uiMSG().error( attrset_->errMsg() ); newdesc->unRef(); return; }

    newList( attrdescs_.size() );
    adsman_->setSaved( false );
    applycb.trigger();
}


Attrib::Desc* uiAttribDescSetEd::createAttribDesc( bool checkuserref )
{
    uiAttrDescEd& curde = activeDescEd();
    BufferString attribname = getAttribName( curde );
    Desc* newdesc = PF().createDescCopy( attribname );
    if ( !newdesc )
	mErrRetNull( tr("Internal: cannot create attribute of type '%1'")
		     .arg(attribname) )

    newdesc->ref();
    newdesc->setDescSet( attrset_ );
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
    removeUnusedAttrDescs();
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


void uiAttribDescSetEd::sortPushCB( CallBacker* )
{
    attrset_->sortDescSet();
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

    removeUnusedAttrDescs();
    if ( saveButtonChecked() && !doSave(true) )
	return false;

    if ( inoutadsman_ )
	inoutadsman_->setSaved( adsman_->isSaved() );

    prevsavestate = saveButtonChecked();
    nmprefgrp_ = attrtypefld_->group();
    applycb.trigger();
    return true;
}


bool uiAttribDescSetEd::rejectOK()
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
    attrlistfld_->addItems( userattrnames_.getUiStringSet() );
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

    attrtypefld_->setAttr( attrnm );
    updateFields( false );
}


BufferString uiAttribDescSetEd::getAttribName( uiAttrDescEd& desced ) const
{
    BufferString attribname = desced.attribName();
    if ( attribname.isEmpty() )
    {
	pErrMsg("Missing uiAttrDescEd attribName()");
	attribname = uiAF().attrNameOf( attrtypefld_->attr() );
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
	const BufferString typenm( curdesc ? curdesc->attribName().str()
					   : "RefTime" );
	attrtypefld_->setAttr( uiAF().dispNameOf(typenm) );
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
	    de.setDescSet( attrset_ );
	else if ( isappropriatedesc )
	    de.setDesc( curdesc, adsman_ );

	de.display( istargetdesced );
    }

    updating_fields_ = false;
}


bool uiAttribDescSetEd::doAcceptInputs()
{
    uiAttrDescEd& curdesced = activeDescEd();
    for ( int idx=0; idx<attrset_->size(); idx++ )
    {
	const DescID descid = attrset_->getID( idx );
	Desc* desc = attrset_->getDesc( descid );
	uiRetVal uirv = curdesced.errMsgs( desc );
	if ( !uirv.isOK() )
	{
	    uiString startmsg( desc->isStored() ? tr("Error for '%1':\n")
			    : tr("Input is not correct for attribute '%1'.") );
	    uiRetVal resuirv( startmsg.arg( desc->userRef() ) );
	    resuirv.add( uirv );
	    uiMSG().error( resuirv );
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
	    DescID id = usedesc->id();
	    TypeSet<DescID> attribids;
	    attrset_->getIds( attribids );
	    int oldattridx = attribids.indexOf( id );
	    Desc* newdesc = createAttribDesc( false );
	    if ( !newdesc )
		return false;

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
	    removeUnusedAttrDescs();
	    adsman_->setSaved( false );
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
    const int selidx = attrlistfld_->currentItem();
    return selidx<0 ? 0 : const_cast<Desc*>( attrdescs_[selidx] );
}


uiAttrDescEd& uiAttribDescSetEd::activeDescEd()
{
    const BufferString attrstr = attrtypefld_->attr();
    BufferString attrnm = uiAF().attrNameOf( attrstr );
    if ( !attrnm )
	{ pErrMsg("Huh"); attrnm = attrstr; }

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

    while ( attrset_->isPresent(attrnm) )
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


bool uiAttribDescSetEd::doSetIO( bool forread )
{
    if ( !setctio_.ioobj_ )
    {
	if ( setid_.isInvalid() ) return false;

	setctio_.ioobj_ = DBM().get( setid_ );
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
	       .arg(setctio_.ioobj_->uiName())
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


void uiAttribDescSetEd::newSetCB( CallBacker* )
{
    if ( !offerSetSave() )
	return;

    adsman_->inputHistory().setEmpty();
    updateFields();

    attrset_->removeAll( true );
    setctio_.ioobj_ = 0;
    setid_.setInvalid();
    updateUserRefs();
    newList( -1 );
    attrsetfld_->setText( sKeyNotSaved );
    adsman_->setSaved( true );
}


void uiAttribDescSetEd::openSetCB( CallBacker* )
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


void uiAttribDescSetEd::openDefSetCB( CallBacker* )
{
    if ( !offerSetSave() ) return;

    BufferStringSet attribfiles;
    BufferStringSet attribnames;
    BufferStringSet errmsgs;
    getDefaultAttribsets( attribfiles, attribnames, errmsgs );

    uiSelectFromList::Setup sflsu( tr("Default Attribute Sets"), attribnames );
    sflsu.dlgtitle( tr("Select default attribute set") );
    uiSelectFromList dlg( this, sflsu );
    dlg.setHelpKey( mODHelpKey(mAttribDescSetEddefaultSetHelpID) );
    if ( dlg.selFld() ) //should not be necessary, yet safer
    {
	for ( int idaf=0; idaf<attribfiles.size(); idaf ++ )
	{
	    if ( !errmsgs.get(idaf).isEmpty() )
		dlg.selFld()->setColor( idaf, Color::LightGrey() );
	}
    }
    if ( !dlg.go() ) return;

    const int selitm = dlg.selection();
    if ( selitm < 0 ) return;
    if ( !errmsgs.get( selitm ).isEmpty() )
    {
	uiMSG().error( tr(errmsgs.get( selitm ).buf()) );
	return; //TODO: can we deliver the message earlier?
		//as some kind of tooltip ?
    }

    const char* filenm = attribfiles[selitm]->buf();

    importFromFile( filenm );
    attrsetfld_->setText( sKeyNotSaved );
}


void uiAttribDescSetEd::loadDefaultAttrSet( const char* attribsetnm )
{
    BufferStringSet attribfiles;
    BufferStringSet attribnames;
    BufferStringSet errmsgs;
    getDefaultAttribsets( attribfiles, attribnames, errmsgs );
    const int selidx = attribnames.indexOf( attribsetnm );
    if ( selidx>=0 )
    {
	if ( !errmsgs.get( selidx ).isEmpty() )
	    uiMSG().error( tr(errmsgs.get( selidx ).buf()) );

	const char* filenm = attribfiles[selidx]->buf();
	importFromFile( filenm );
	attrsetfld_->setText( sKeyNotSaved );
    }
}


static void gtDefaultAttribsets( const char* dirnm, bool is2d,
				 BufferStringSet& attribfiles,
				 BufferStringSet& attribnames,
				 BufferStringSet& errmsgs )
{
    if ( !dirnm || !File::exists(dirnm) )
	return;

    DirList attrdl( dirnm, File::DirsInDir, "*Attribs" );
    for ( int idx=0; idx<attrdl.size(); idx++ )
    {
	File::Path fp( dirnm, attrdl.get(idx), "index" );
	IOPar iopar("AttributeSet Table");
	iopar.read( fp.fullPath(), sKey::Pars(), false );
	PtrMan<IOPar> subpar = iopar.subselect( is2d ? "2D" : "3D" );
	if ( !subpar ) continue;

	for ( int idy=0; idy<subpar->size(); idy++ )
	{
	    BufferString attrfnm = subpar->getValue( idy );
	    if ( !File::Path(attrfnm).isAbsolute() )
	    {
		fp.setFileName( attrfnm );
		attrfnm = fp.fullPath();
	    }

	    if ( !File::exists(attrfnm) ) continue;
	    od_istream strm( attrfnm );
	    ascistream ascstrm( strm );
	    IOPar attrsetiopar( ascstrm );
	    DescSet tmpds( is2d );
	    tmpds.usePar( attrsetiopar );
	    uiRetVal retvaldefds;
	    for ( int idd=0; idd<tmpds.nrDescs(true,true); idd++ )
	    {
		if ( tmpds.desc(idd) )
		{
		    Attrib::Provider* prov =
				    PF().create( *tmpds.desc(idd), true );
		    if ( !prov ) continue;
		    retvaldefds.add ( prov->isActive() );
		}
	    }
	    attribnames.add( subpar->getKey(idy) );
	    attribfiles.add( attrfnm );
	    errmsgs.add( retvaldefds.getText() );
	}
    }
}


void uiAttribDescSetEd::getDefaultAttribsets( BufferStringSet& attribfiles,
					      BufferStringSet& attribnames,
					      BufferStringSet& errmsgs )
{
    const bool is2d = adsman_ ? adsman_->is2D() : attrset_->is2D();
    gtDefaultAttribsets( mGetApplSetupDataDir(), is2d, attribfiles,
			 attribnames, errmsgs );
    gtDefaultAttribsets( mGetSWDirDataDir(), is2d, attribfiles, attribnames,
			 errmsgs );
}


void uiAttribDescSetEd::showMatrixCB( CallBacker* )
{
    const HelpKey key( WebsiteHelp::sKeyFactoryName(),
		       WebsiteHelp::sKeyAttribMatrix() );
    HelpProvider::provideHelp( key );
}


void uiAttribDescSetEd::importFromSeisCB( CallBacker* )
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

    File::Path fp( ioobj->mainFileName() );
    fp.setExtension( sProcFileExtension() );
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


void uiAttribDescSetEd::importSetCB( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiSelObjFromOtherSurvey objsel( this, setctio_.ctxt_ );
    objsel.setHelpKey( mODHelpKey(mAttribDescSetEdimportSetHelpID) );
    if ( objsel.go() )
    {
	IOObj* oldioobj = setctio_.ioobj_;
	setctio_.ioobj_ = objsel.ioObj()->clone();
	setctio_.ioobj_->setKey( DBKey::getInvalid() );
	if ( doSetIO(true) )
	{
	    delete oldioobj;
	    setctio_.setObj(
			DBM().getByName(IOObjContext::Attr,setctio_.name()) );
	    if ( setctio_.ioobj_ )
		setid_ = setctio_.ioobj_->key();
	    else
		setid_.setInvalid();
	    replaceStoredAttr();
	    newList( -1 );
	    attrsetfld_->setText( sKeyNotSaved );
	    applycb.trigger();
	}
    }
}


void uiAttribDescSetEd::importFileCB( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiGetFileForAttrSet dlg( this, true, inoutadsman_->is2D() );
    if ( dlg.go() )
	importFromFile( dlg.fileName() );
}


void uiAttribDescSetEd::job2SetCB( CallBacker* )
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


void uiAttribDescSetEd::crossPlotCB( CallBacker* )
{
    if ( adsman_ && adsman_->descSet() )
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

    if ( !doCommit() ) return;

    uiAttrVolOut* dlg = new uiAttrVolOut( this, *attrset_, false,
					  0, DBKey::getInvalid() );
    dlg->setInput( curDesc()->id() );
    dlg->show();
}


void uiAttribDescSetEd::evalAttributeCB( CallBacker* )
{
    if ( !doCommit() ) return;
    evalattrcb.trigger();
}


void uiAttribDescSetEd::crossEvalAttrsCB( CallBacker* )
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


void uiAttribDescSetEd::chgAttrInputsCB( CallBacker* )
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


void uiAttribDescSetEd::removeUnusedAttrDescs()
{
     if ( attrset_ )
	 attrset_->removeUnused( true );
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
{ return dotfld_->fileName(); }


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
    if ( !attrset_ || attrlistfld_->isEmpty() ) return;

    BufferString dotpath;
    Settings::common().get( "Dot path", dotpath );
    if ( dotpath.isEmpty() )
    {
	uiWhereIsDotDlg dlg( this );
	if ( !dlg.go() ) return;

	dotpath = dlg.fileName();
    }

    const BufferString fnm = File::Path::getTempName( "dot" );
    const char* attrnm = DBM().nameOf( setid_ );
    attrset_->exportToDot( attrnm, fnm );

    File::Path outputfp( fnm );
    outputfp.setExtension( "png" );
    BufferString cmd( "\"", dotpath, "\"" );
    cmd.add( " -Tpng " ).add( fnm ).add( " -o " ).add( outputfp.fullPath() );
    const bool res = OS::ExecCommand( cmd.buf() );
    if ( !res )
    {
	uiMSG().error( tr("Could not execute %1").arg(cmd) );
	return;
    }

    uiDesktopServices::openUrl( outputfp.fullPath() );
}


void uiAttribDescSetEd::updateCurDescEd()
{
    curDescEd().setDesc( curDesc(), adsman_ );
}


void uiAttribDescSetEd::updateAllDescsDefaults()
{
    PF().updateAllDescsDefaults();
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
	const Attrib::Desc* ad = attrset_->getDesc( adids[idx] );
	const BufferString& attrnm = ad->attribName();
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
