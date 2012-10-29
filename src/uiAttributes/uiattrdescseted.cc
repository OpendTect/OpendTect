/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "ptrman.h"
#include "pickset.h"
#include "seistype.h"
#include "survinfo.h"
#include "settings.h"
#include "strmprov.h"

#include "uiattrdesced.h"
#include "uiattrgetfile.h"
#include "uiattribfactory.h"
#include "uiattrinpdlg.h"
#include "uiattrtypesel.h"
#include "uiautoattrdescset.h"
#include "uievaluatedlg.h"
#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiselobjothersurv.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uistoredattrreplacer.h"
#include "uitextedit.h"
#include "uitoolbar.h"


const char* uiAttribDescSetEd::sKeyUseAutoAttrSet = "dTect.Auto Attribute set";
const char* uiAttribDescSetEd::sKeyAuto2DAttrSetID = "2DAttrset.Auto ID";
const char* uiAttribDescSetEd::sKeyAuto3DAttrSetID = "3DAttrset.Auto ID";

BufferString uiAttribDescSetEd::nmprefgrp_( "" );
static const char* sKeyNotSaved = "<not saved>";

static bool prevsavestate = true;

using namespace Attrib;

uiAttribDescSetEd::uiAttribDescSetEd( uiParent* p, DescSetMan* adsm,
				      const char* prefgrp, bool attrsneedupdt )
    : uiDialog(p,uiDialog::Setup( adsm && adsm->is2D() ? "Attribute Set 2D"
					: "Attribute Set 3D","","101.1.0")
	.savebutton(true).savetext("Save on OK  ")
	.menubar(true)
	.modal(false))
    , inoutadsman_(adsm)
    , userattrnames_(*new BufferStringSet)
    , setctio_(*mMkCtxtIOObj(AttribDescSet))
    , prevdesc_(0)
    , attrset_(0)
    , dirshowcb(this)
    , evalattrcb(this)
    , crossevalattrcb(this)
    , xplotcb(this)
    , adsman_(0)
    , updating_fields_(false)
    , attrsneedupdt_(attrsneedupdt)
{
    setctio_.ctxt.toselect.dontallow_.set( sKey::Type(),
	    				   adsm->is2D() ? "3D" : "2D" );

    createMenuBar();
    createToolBar();
    createGroups();
    attrtypefld_->setGrp( prefgrp ? prefgrp : nmprefgrp_.buf() );

    init();
}


#define mInsertItemNoIcon( txt, func ) \
{ \
    uiMenuItem* itm = new uiMenuItem(txt,mCB(this,uiAttribDescSetEd,func));\
    filemnu->insertItem( itm ); \
}

#define mInsertItem( txt, func, fnm ) \
{ \
    uiMenuItem* itm = new uiMenuItem(txt,mCB(this,uiAttribDescSetEd,func),fnm);\
    filemnu->insertItem( itm ); \
}

void uiAttribDescSetEd::createMenuBar()
{
    uiMenuBar* menu = menuBar();
    if( !menu )		{ pErrMsg("huh?"); return; }

    uiPopupMenu* filemnu = new uiPopupMenu( this, "&File" );
    mInsertItem( "&New set ...", newSet, "newset" );
    mInsertItem( "&Open set ...", openSet, "openset" );
    mInsertItem( "&Save set ...", savePush, "save" );
    mInsertItem( "&Save set as ...", saveAsPush, "saveas" );
    mInsertItemNoIcon( "&Auto Load Attribute Set ...", autoSet );
    mInsertItemNoIcon( "&Change input ...", changeInput );
    filemnu->insertSeparator();
    mInsertItem( "Open &Default set ...", defaultSet, "defset" );
    mInsertItem( "&Import set ...", importSet, "impset" );
    mInsertItemNoIcon( "Import set from &file ...", importFile );
    mInsertItem( "&Reconstruct set from job file ...", job2Set, "job2set" );

    menu->insertItem( filemnu );
}


#define mAddButton(pm,func,tip) \
    toolbar_->addButton( pm, tip, mCB(this,uiAttribDescSetEd,func) )

void uiAttribDescSetEd::createToolBar()
{
    toolbar_ = new uiToolBar( this, "AttributeSet tools" );
    mAddButton( "newset", newSet, "New attribute set" );
    mAddButton( "openset", openSet, "Open attribute set" );
    mAddButton( "defset", defaultSet, "Open default attribute set" );
    mAddButton( "impset", importSet, 
	    	"Import attribute set from other survey" );
    mAddButton( "job2set", job2Set, "Reconstruct set from job file" );
    mAddButton( "save", savePush, "Save attribute set" );
    mAddButton( "saveas", saveAsPush, "Save attribute set as" );
    toolbar_->addSeparator();
    mAddButton( "showattrnow", directShow, 
	    	"Redisplay element with current attribute");
    mAddButton( "evalattr", evalAttribute, "Evaluate attribute" );
    mAddButton( "evalcrossattr",crossEvalAttrs,"Cross attributes evaluate");
    mAddButton( "xplot", crossPlot, "Cross-Plot attributes" );
}


void uiAttribDescSetEd::createGroups()
{
//  Left part
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    attrsetfld_ = new uiGenInput( leftgrp, "Attribute set", StringInpSpec() );
    attrsetfld_->setReadOnly( true );

    attrlistfld_ = new uiListBox( leftgrp, "Defined Attributes" );
    attrlistfld_->setStretch( 2, 2 );
    attrlistfld_->selectionChanged.notify( mCB(this,uiAttribDescSetEd,selChg) );
    attrlistfld_->attach( leftAlignedBelow, attrsetfld_ );

    moveupbut_ = new uiToolButton( leftgrp, uiToolButton::UpArrow, "Up",
				    mCB(this,uiAttribDescSetEd,moveUpDownCB) );
    moveupbut_->attach( centeredRightOf, attrlistfld_ );
    movedownbut_ = new uiToolButton( leftgrp, uiToolButton::DownArrow, "Down",
				    mCB(this,uiAttribDescSetEd,moveUpDownCB) );
    movedownbut_->attach( alignedBelow, moveupbut_ );
    sortbut_ = new uiToolButton( leftgrp, "sort", "Sort attributes",
	    			 mCB(this,uiAttribDescSetEd,sortPush) );
    sortbut_->attach( alignedBelow, movedownbut_ );
    rmbut_ = new uiToolButton( leftgrp, "trashcan", "Remove selected",
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
	desceds_ += de;
	de->attach( alignedBelow, attrtypefld_ );
    }
    attrtypefld_->update();
    degrp->setHAlignObj( attrtypefld_ );

    helpbut_ = new uiToolButton( degrp, "contexthelp", "Help",
				mCB(this,uiAttribDescSetEd,helpButPush) );
    helpbut_->attach( rightTo, attrtypefld_ );

    attrnmfld_ = new uiGenInput( rightgrp, "Attribute name" );
    attrnmfld_->attach( alignedBelow, degrp );
    attrnmfld_->updateRequested.notify( mCB(this,uiAttribDescSetEd,addPush) );

    addbut_ = new uiPushButton( rightgrp, "Add as new", true );
    addbut_->attach( alignedBelow, attrnmfld_ );
    addbut_->activated.notify( mCB(this,uiAttribDescSetEd,addPush) );

    revbut_ = new uiPushButton( rightgrp, "Revert changes", true );
    revbut_->attach( rightTo, addbut_ );
    revbut_->attach( rightBorder );
    revbut_->activated.notify( mCB(this,uiAttribDescSetEd,revPush) );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


#define mUnsetAuto \
    SI().getPars().removeSingle( autoidkey ); \
    SI().savePars()

void uiAttribDescSetEd::init()
{
    delete attrset_; attrset_ = 0;
    attrset_ = new Attrib::DescSet( *inoutadsman_->descSet() );
    adsman_ = new DescSetMan( inoutadsman_->is2D(), attrset_ );
    adsman_->fillHist();
    adsman_->setSaved( inoutadsman_->isSaved() );

    setid_ = inoutadsman_->attrsetid_;
    IOM().to( setctio_.ctxt.getSelKey() );
    setctio_.setObj( IOM().get(setid_) );
    bool autoset = false;
    MultiID autoid;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, autoset );
    const char* autoidkey = is2D() ? uiAttribDescSetEd::sKeyAuto2DAttrSetID
				   : uiAttribDescSetEd::sKeyAuto3DAttrSetID;
    if ( autoset && SI().pars().get(autoidkey,autoid) && autoid != setid_ )
    {
        BufferString msg = "The Attribute-set selected for Auto-load ";
        msg += "is no longer valid\n";
        msg += "Load another now?";
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
			MultiID id = ioobj ? ioobj->key() : "";
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
	const BufferString txt = setctio_.ioobj ? setctio_.ioobj->name().buf()
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
    delete toolbar_;
    delete adsman_;
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
    setctio_.ctxt.forread = false;
    IOObj* oldioobj = setctio_.ioobj;
    bool needpopup = !oldioobj || !endsave;
    if ( needpopup )
    {
	uiIOObjSelDlg dlg( this, setctio_ );
	if ( !dlg.go() || !dlg.ioObj() ) return false;

	setctio_.ioobj = 0;
	setctio_.setObj( dlg.ioObj()->clone() );
    }

    if ( !doSetIO( false ) )
    {
	if ( oldioobj != setctio_.ioobj )
	    setctio_.setObj( oldioobj );
	return false;
    }

    if ( oldioobj != setctio_.ioobj )
	delete oldioobj;
    setid_ = setctio_.ioobj->key();
    if ( !endsave )
	attrsetfld_->setText( setctio_.ioobj->name() );
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
	is2d ? par.set(uiAttribDescSetEd::sKeyAuto2DAttrSetID, (const char*)id)
	     : par.set(uiAttribDescSetEd::sKeyAuto3DAttrSetID, (const char*)id);
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
}


Attrib::Desc* uiAttribDescSetEd::createAttribDesc( bool checkuref )
{
    BufferString newnmbs( attrnmfld_->text() );
    char* newnm = newnmbs.buf();
    mSkipBlanks( newnm );
    removeTrailingBlanks( newnm );
    if ( checkuref && !validName(newnm) ) return 0;

    uiAttrDescEd* curde = 0;
    if ( !(curde = curDescEd()) )
	mErrRetNull( "Cannot add without a valid attribute type" )

    BufferString attribname = curde->attribName();
    if ( attribname.isEmpty() )
	attribname = uiAF().attrNameOf( attrtypefld_->attr() );

    Desc* newdesc = PF().createDescCopy( attribname );
    if ( !newdesc )
	mErrRetNull( "Cannot create attribdesc" )

    newdesc->setDescSet( attrset_ );
    newdesc->ref();
    const char* res = curde->commit( newdesc );
    if ( res ) { newdesc->unRef(); mErrRetNull( res ); }
    newdesc->setUserRef( newnm );
    return newdesc;
}


void uiAttribDescSetEd::helpButPush( CallBacker* )
{
    uiAttrDescEd* curdesced = curDescEd();
    const char* helpid = curdesced->helpID();
    uiMainWin::provideHelp( helpid );
}


void uiAttribDescSetEd::rmPush( CallBacker* )
{
    Desc* curdesc = curDesc();
    if ( !curdesc ) return;

    if ( attrset_->isAttribUsed( curdesc->id() ) )
    {
	uiMSG().error( "Cannot remove this attribute. It is used\n"
		       "as input for another attribute" );
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
    sortbut_->setSensitive( size > 1);;
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
    revbut_->setSensitive( havedescs );
}


bool uiAttribDescSetEd::acceptOK( CallBacker* )
{
    if ( !doCommit() )
	return false;
    removeNotUsedAttr();
    if ( saveButtonChecked() && !doSave(true) )
	return false;
    
    if ( inoutadsman_ )
        inoutadsman_->setSaved( adsman_->isSaved() );

    prevsavestate = saveButtonChecked();
    nmprefgrp_ = attrtypefld_->group();
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


void uiAttribDescSetEd::updateFields( bool set_type )
{
    updating_fields_ = true;
    updateAttrName();
    Desc* desc = curDesc();
    uiAttrDescEd* curde = 0;
    if ( set_type )
    {
	BufferString typenm = desc ? desc->attribName() : "RefTime";
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
	BufferString msg = "This will change the type of existing attribute '";
	msg += usedesc->userRef();
	msg += "'.\nThis will remove previous definition of the attribute.\n";
	msg +="If you want to avoid this please use 'Cancel' and 'Add as new'.";
	msg += "\nAre you sure you want to change the attribute type?";
	bool res = uiMSG().askGoOn( msg, "Change", "Cancel" );
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
	    if ( attrset_->errMsg() )
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

    const char* res = curdesced->commit();
    if ( res )
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

    int newselidx = userattrnames_.indexOf( selnm );
    if ( newselidx < 0 ) newselidx = 0;
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
    if ( !attrnm ) attrnm = attrstr;

    uiAttrDescEd* ret = 0;
    for ( int idx=0; idx<desceds_.size(); idx++ )
    {
	uiAttrDescEd* de = desceds_[idx];
	if ( !de ) continue;

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
    if ( !isalnum(newnm[0]) )
	mErrRetFalse( "Please start attribute name with a letter or number" );

    if ( strchr(newnm,'!') || strchr(newnm,':') || strchr(newnm,';') ||
	 strchr(newnm,'#') )
	mErrRetFalse( "Attribute name may not contain '!', '#', ';' or ':'." );

    if ( strlen(newnm) < 2 )
	mErrRetFalse( "Please enter a name of at least 2 characters." );

    TypeSet<DescID> ids;
    attrset_->getIds( ids );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const Desc& ad = *attrset_->getDesc( ids[idx] );
	if ( !strcmp(ad.userRef(),newnm) )
	{
	    uiMSG().error( "The name you entered for the attribute already"
			  " exists.\nPlease choose another name." );
	    return false;
	}
    }

    return true;
}


bool uiAttribDescSetEd::setUserRef( Desc* attrdesc )
{
    BufferString usertypednm( attrnmfld_->text() );
    char* ptr = usertypednm.buf();
    mTrimBlanks( ptr );
    const BufferString newnm( ptr );

    if ( newnm == attrdesc->userRef() ) return true;
    else if ( !validName(newnm) ) return false;

    const char* res = curDescEd()->commit();
    if ( res )
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
    if ( !setctio_.ioobj )
    {
	if ( setid_.isEmpty() ) return false;

	setctio_.ioobj = IOM().get( setid_ );
	if ( !setctio_.ioobj ) 
	    mErrRetFalse("Cannot find attribute set in data base")
    }

    BufferString bs;
    if ( forread )
    {
	Attrib::DescSet attrset( is2D() );
	if ( !AttribDescSetTranslator::retrieve(attrset,setctio_.ioobj,bs) )
	    mErrRetFalse(bs)

	if ( attrset.is2D() != is2D() )
	{
	    bs = "Attribute Set "; bs += setctio_.ioobj->name();
	    bs += " is of type "; bs += attrset.is2D() ? "2D" : "3D";
	    mErrRetFalse(bs.buf())
	}

	*attrset_ = attrset;
	adsman_->setDescSet( attrset_ );
	adsman_->fillHist();
    }
    else if ( !AttribDescSetTranslator::store(*attrset_,setctio_.ioobj,bs) )
	mErrRetFalse(bs)

    if ( !bs.isEmpty() )
	pErrMsg( bs );

    setid_ = setctio_.ioobj->key();
    return true;
}


void uiAttribDescSetEd::newSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    adsman_->inputHistory().setEmpty();
    updateFields();

    attrset_->removeAll( true );
    setctio_.ioobj = 0;
    setid_ = -1;
    updateUserRefs();
    newList( -1 );
    attrsetfld_->setText( sKeyNotSaved );
    adsman_->setSaved( true );
}


void uiAttribDescSetEd::openSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    setctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, setctio_ );
    if ( dlg.go() && dlg.ioObj() )
	openAttribSet( dlg.ioObj() );

}

void uiAttribDescSetEd::openAttribSet( const IOObj* ioobj )
{
    if ( !ioobj ) return;
    IOObj* oldioobj = setctio_.ioobj; setctio_.ioobj = 0;
    setctio_.setObj( ioobj->clone() );
    if ( !doSetIO( true ) )
	setctio_.setObj( oldioobj );
    else
    {
	delete oldioobj;
	setid_ = setctio_.ioobj->key();
	if ( attrset_->couldBeUsedInAnyDimension() )
	    replaceStoredAttr();

	newList( -1 );
	attrsetfld_->setText( setctio_.ioobj->name() );
	adsman_->setSaved( true );
	TypeSet<DescID> ids;
	attrset_->getIds( ids );
	for ( int idx=0; idx<attrset_->size(); idx++ )
	{
	    Desc* ad = attrset_->getDesc( ids[idx] );
	    if ( !ad ) continue;
	    if ( ad->isStored() && ad->isSatisfied()==2 )
	    {
		BufferString msg = "The attribute: '";
		msg += ad->userRef();
		msg += "' will be removed\n";
		msg += "Storage ID is no longer valid";
		uiMSG().message( msg );
		attrset_->removeDesc( ad->id() );
		idx--;
	    }
	}
    }
}


void uiAttribDescSetEd::defaultSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    BufferStringSet attribfiles;
    BufferStringSet attribnames;
    getDefaultAttribsets( attribfiles, attribnames );

    uiSelectFromList::Setup sflsu( "Default Attribute Sets", attribnames );
    sflsu.dlgtitle( "Select default attribute set" );
    uiSelectFromList dlg( this, sflsu );
    dlg.setHelpID("101.1.6");
    if ( !dlg.go() ) return;
    
    const int selitm = dlg.selection();
    if ( selitm < 0 ) return;
    const char* filenm = attribfiles[selitm]->buf();

    importFromFile( filenm );
    attrsetfld_->setText( sKeyNotSaved );
}


static void gtDefaultAttribsets( const char* dirnm, bool is2d,
				 BufferStringSet& attribfiles,
				 BufferStringSet& attribnames )
{
    if ( !dirnm || !File::exists(dirnm) )
	return;

    DirList attrdl( dirnm, DirList::DirsOnly, "*Attribs" );
    for ( int idx=0; idx<attrdl.size(); idx++ )
    {
	FilePath fp( dirnm, attrdl.get(idx), "index" );
	IOPar iopar("AttributeSet Table");
	iopar.read( fp.fullPath(), sKey::Pars(), false );
	PtrMan<IOPar> subpar = iopar.subselect( is2d ? "2D" : "3D" );
	if ( !subpar ) continue;

	for ( int idy=0; idy<subpar->size(); idy++ )
	{
	    BufferString attrfnm = subpar->getValue( idy );
	    if ( !FilePath(attrfnm).isAbsolute() )
	    {
		fp.setFileName( attrfnm );
		attrfnm = fp.fullPath();
	    }

	    if ( !File::exists(attrfnm) ) continue;
		
	    attribnames.add( subpar->getKey(idy) );
	    attribfiles.add( attrfnm );
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


void uiAttribDescSetEd::importFromFile( const char* filenm )
{
    StreamProvider sp( filenm );
    ascistream ascstrm( *sp.makeIStream(false).istrm );
    IOPar iopar( ascstrm );
    replaceStoredAttr( iopar );
    attrset_->usePar( iopar, toFloat(ascstrm.version()) );
    newList( -1 );
    attrsetfld_->setText( sKeyNotSaved );
    setctio_.ioobj = 0;
}


void uiAttribDescSetEd::importSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiSelObjFromOtherSurvey objdlg( this, setctio_ );
    objdlg.setHelpID( "0.3.5" );
    IOObj* oldioobj = setctio_.ioobj; setctio_.ioobj = 0;
    if ( objdlg.go() && setctio_.ioobj )
    {
	if ( !doSetIO( true ) )
	    setctio_.setObj( oldioobj );
	else
	{
	    delete oldioobj;
	    setid_ = setctio_.ioobj->key();
	    replaceStoredAttr();
	    newList( -1 );
	    attrsetfld_->setText( sKeyNotSaved );
	    setctio_.ioobj = 0;
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
    if ( dlg.go() )
    {
	if ( dlg.attrSet().nrDescs(false,false) < 1 )
	    mErrRet( "No usable attributes in file" )

	*attrset_ = dlg.attrSet();
	adsman_->setSaved( false );

	setctio_.setObj( 0 );
	newList( -1 ); attrsetfld_->setText( sKeyNotSaved );
    }
}


void uiAttribDescSetEd::crossPlot( CallBacker* )
{
    if ( !adsman_ || !adsman_->descSet() ) return;
    xplotcb.trigger();
}


void uiAttribDescSetEd::directShow( CallBacker* )
{
    if ( !curDesc() )
	mErrRet( "Please add this attribute first" )

    if ( doCommit() )
	dirshowcb.trigger();
    updateFields();
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

    BufferString msg( "Attribute set is not saved.\nSave now?" );
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

    TypeSet<Attrib::DescID> adids;
    curDesc()->getDependencies( adids );
    adids.insert( 0, curDesc()->id() );

    TypeSet<int> ids;
    TypeSet<EvalParam> eps;

    for ( int idx=0; idx<adids.size(); idx++ )
    {
	const Attrib::Desc* ad = attrset_->getDesc( adids[idx] );
	const char* attrnm = ad->attribName();
	const char* usernm = ad->userRef();
	for ( int idy=0; idy<desceds_.size(); idy++ )
    	{
    	    if ( !desceds_[idy] || strcmp(attrnm,desceds_[idy]->attribName()) )
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
