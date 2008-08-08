/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.cc,v 1.75 2008-08-08 11:00:44 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiattrdescseted.h"

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
#include "filegen.h"
#include "filepath.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "seistype.h"
#include "survinfo.h"
#include "settings.h"

#include "uiattrdesced.h"
#include "uiattrgetfile.h"
#include "uiattribcrossplot.h"
#include "uiattribfactory.h"
#include "uiattrinpdlg.h"
#include "uiattrtypesel.h"
#include "uiautoattrdescset.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilineedit.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uistoredattrreplacer.h"
#include "uitextedit.h"
#include "uitoolbar.h"


const char* uiAttribDescSetEd::sKeyUseAutoAttrSet = "dTect.Auto Attribute set";
const char* uiAttribDescSetEd::sKeyAuto2DAttrSetID = "2DAttrset.Auto ID";
const char* uiAttribDescSetEd::sKeyAuto3DAttrSetID = "3DAttrset.Auto ID";

BufferString uiAttribDescSetEd::nmprefgrp( "" );

static bool prevsavestate = true;
static bool evaldlgpoppedup = false;

using namespace Attrib;

uiAttribDescSetEd::uiAttribDescSetEd( uiParent* p, DescSetMan* adsm,
				      const char* prefgrp )
    : uiDialog(p,uiDialog::Setup( adsm && adsm->is2D() ? "Attribute Set 2D"
					: "Attribute Set 3D","","101.1.0")
	.savebutton(true).savetext("Save on OK  ")
	.menubar(true)
	.modal(false))
    , inoutadsman(adsm)
    , userattrnames(*new BufferStringSet)
    , setctio(*mMkCtxtIOObj(AttribDescSet))
    , prevdesc(0)
    , attrset(0)
    , dirshowcb(this)
    , evalattrcb(this)
    , adsman(0)
    , updating_fields(false)
{
    createMenuBar();
    createToolBar();
    createGroups();
    attrtypefld->setGrp( prefgrp ? prefgrp : nmprefgrp.buf() );

    init();
}


#define mInsertItem( txt, func ) \
    filemnu->insertItem( new uiMenuItem(txt,mCB(this,uiAttribDescSetEd,func)) )

void uiAttribDescSetEd::createMenuBar()
{
    uiMenuBar* menu = menuBar();
    if( !menu )		{ pErrMsg("huh?"); return; }

    uiPopupMenu* filemnu = new uiPopupMenu( this, "&File" );
    mInsertItem( "&New set ...", newSet );
    mInsertItem( "&Open set ...", openSet );
    mInsertItem( "&Save set ...", savePush );
    mInsertItem( "&Auto Load Attribute Set ...", autoSet );
    mInsertItem( "&Change input ...", changeInput );
    filemnu->insertSeparator();
    mInsertItem( "Open &Default set ...", defaultSet );
    mInsertItem( "&Import set ...", importSet );
    mInsertItem( "Import set from &file ...", importFile );
    mInsertItem( "&Reconstruct set from job file ...", job2Set );

    menu->insertItem( filemnu );
}


#define mAddButton(pm,func,tip) \
    toolbar->addButton( pm, mCB(this,uiAttribDescSetEd,func), tip )

void uiAttribDescSetEd::createToolBar()
{
    toolbar = new uiToolBar( this, "AttributeSet tools" );
    mAddButton( "newset.png", newSet, "New attribute set" );
    mAddButton( "openset.png", openSet, "Open attribute set" );
    mAddButton( "defset.png", defaultSet, "Open default attribute set" );
    mAddButton( "impset.png", importSet, 
	    	"Import attribute set from other survey" );
    mAddButton( "job2set.png", job2Set, "Reconstruct set from job file" );
    mAddButton( "saveset.png", savePush, "Save attribute set" );
    toolbar->addSeparator();
    mAddButton( "showattrnow.png", directShow, 
	    	"Redisplay element with current attribute");
    mAddButton( "evalattr.png", evalAttribute, "Evaluate attribute" );
    mAddButton( "xplot.png", crossPlot, "Cross-Plot attributes" );
}


void uiAttribDescSetEd::createGroups()
{
//  Left part
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    attrsetfld = new uiGenInput( leftgrp, "Attribute set", StringInpSpec() );
    attrsetfld->setReadOnly( true );

    attrlistfld = new uiListBox( leftgrp, "Defined Attributes" );
    attrlistfld->attach( ensureBelow, attrsetfld );
    attrlistfld->setStretch( 2, 2 );
    attrlistfld->selectionChanged.notify( mCB(this,uiAttribDescSetEd,selChg) );

    rmbut = new uiPushButton( leftgrp, "Remove selected", true );
    rmbut->attach( centeredBelow, attrlistfld );
    rmbut->activated.notify( mCB(this,uiAttribDescSetEd,rmPush) );

//  Right part
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );

    uiGroup* degrp = new uiGroup( rightgrp, "DescEdGroup" );
    degrp->setStretch( 1, 1 );

    attrtypefld = new uiAttrTypeSel( degrp );
    attrtypefld->selChg.notify( mCB(this,uiAttribDescSetEd,attrTypSel) );
    desceds.allowNull();
    for ( int idx=0; idx<uiAF().size(); idx++ )
    {
	uiAttrDescEd::DomainType dt =
	    	(uiAttrDescEd::DomainType)uiAF().domainType( idx );
	if ( (dt == uiAttrDescEd::Depth && SI().zIsTime())
		|| (dt == uiAttrDescEd::Time && !SI().zIsTime()) )
	    continue;

	const char* attrnm = uiAF().getDisplayName(idx);
	attrtypefld->add( uiAF().getGroupName(idx), attrnm );
	const bool is2d = inoutadsman ? inoutadsman->is2D() : false;
	uiAttrDescEd* de = uiAF().create( degrp, attrnm, is2d, true );
	desceds += de;
	de->attach( alignedBelow, attrtypefld );
    }
    attrtypefld->update();
    degrp->setHAlignObj( attrtypefld );

    const ioPixmap pixmap( "contexthelp.png" );
    helpbut = new uiToolButton( degrp, "&Help button", pixmap );
    helpbut->activated.notify( mCB(this,uiAttribDescSetEd,helpButPush) );
    helpbut->attach( rightTo, attrtypefld );

    attrnmfld = new uiLineEdit( rightgrp, "Attribute name" );
    attrnmfld->setHSzPol( uiObject::Medium );
    attrnmfld->attach( alignedBelow, degrp );
    attrnmfld->returnPressed.notify( mCB(this,uiAttribDescSetEd,addPush) );
    uiLabel* lbl = new uiLabel( rightgrp, "Attribute name" );
    lbl->attach( leftOf, attrnmfld );

    addbut = new uiPushButton( rightgrp, "Add as new", true );
    addbut->attach( alignedBelow, attrnmfld );
    addbut->activated.notify( mCB(this,uiAttribDescSetEd,addPush) );

    revbut = new uiPushButton( rightgrp, "Revert changes", true );
    revbut->attach( rightTo, addbut );
    revbut->attach( rightBorder );
    revbut->activated.notify( mCB(this,uiAttribDescSetEd,revPush) );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


#define mSetAuto( yn ) \
    autoset = yn; \
    Settings::common().setYN(uiAttribDescSetEd::sKeyUseAutoAttrSet, yn); \
    Settings::common().write();

void uiAttribDescSetEd::init()
{
    delete attrset; attrset = 0;
    attrset = inoutadsman->descSet()->clone();
    adsman = new DescSetMan( inoutadsman->is2D(), attrset );
    adsman->fillHist();
    adsman->setSaved( inoutadsman->isSaved() );

    setid = inoutadsman->attrsetid_;
    bool autoset = false;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, autoset );
    IOM().to( setctio.ctxt.getSelKey() );
    setctio.setObj( IOM().get(setid) );
    if ( autoset && !setctio.ioobj )
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
			const bool is2d = is2D();
			IOPar& par = SI().getPars();
			is2d ? par.set( uiAttribDescSetEd::sKeyAuto2DAttrSetID,
					(const char*)id)
			     : par.set( uiAttribDescSetEd::sKeyAuto3DAttrSetID,
				        (const char*)id);
			SI().savePars();
		    }
		    else
		    {
			Settings::common().setYN(uiAttribDescSetEd::
				sKeyUseAutoAttrSet, false);
			Settings::common().write();
		    }
		    openAttribSet( ioobj );
		}
		else
		{
		    const char* filenm = dlg.getAttribfile();
		    const char* attribnm = dlg.getAttribname();
		    importFromFile( filenm );
		    attrsetfld->setText( attribnm );
		    Settings::common().setYN(uiAttribDescSetEd::
			        sKeyUseAutoAttrSet, false);
		    Settings::common().write();
		}
	    }
	    else mSetAuto( false );
	}
	else mSetAuto( false );
    }
    else
    	attrsetfld->setText( setctio.ioobj ? setctio.ioobj->name() : "" );

    cancelsetid = setid;
    newList(0);

    setSaveButtonChecked( prevsavestate );
}


uiAttribDescSetEd::~uiAttribDescSetEd()
{
    delete &userattrnames;
    delete &setctio;
    delete toolbar;
    delete adsman;
}


void uiAttribDescSetEd::setSensitive( bool yn )
{
    topGroup()->setSensitive( yn );
    menuBar()->setSensitive( yn );
    toolbar->setSensitive( yn );
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
    if ( updating_fields ) return;
    	// Fix for continuous call during re-build of list

    doCommit( true );
    updateFields();
    prevdesc = curDesc();
}


void uiAttribDescSetEd::savePush( CallBacker* )
{
    removeNotUsedAttr();
    doSave( false );
}


bool uiAttribDescSetEd::doSave( bool endsave )
{
    doCommit();
    setctio.ctxt.forread = false;
    IOObj* oldioobj = setctio.ioobj;
    bool needpopup = !oldioobj || !endsave;
    if ( needpopup )
    {
	uiIOObjSelDlg dlg( this, setctio );
	if ( !dlg.go() || !dlg.ioObj() ) return false;

	setctio.ioobj = 0;
	setctio.setObj( dlg.ioObj()->clone() );
    }

    if ( !doSetIO( false ) )
    {
	if ( oldioobj != setctio.ioobj )
	    setctio.setObj( oldioobj );
	return false;
    }

    if ( oldioobj != setctio.ioobj )
	delete oldioobj;
    setid = setctio.ioobj->key();
    if ( !endsave )
	attrsetfld->setText( setctio.ioobj->name() );
    adsman->setSaved( true );
    return true;
}


void uiAttribDescSetEd::autoSet()
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
    if ( attrset->addDesc(newdesc) < 0 )
	{ uiMSG().error( attrset->errMsg() ); newdesc->unRef(); return; }

    newList( attrdescs.size() );
    adsman->setSaved( false );
}


Attrib::Desc* uiAttribDescSetEd::createAttribDesc( bool checkuref )
{
    BufferString newnmbs( attrnmfld->text() );
    char* newnm = newnmbs.buf();
    mSkipBlanks( newnm );
    removeTrailingBlanks( newnm );
    if ( checkuref && !validName(newnm) ) return 0;

    uiAttrDescEd* curde = 0;
    if ( !(curde = curDescEd()) )
	mErrRetNull( "Cannot add without a valid attribute type" )

    BufferString attribname = curde->attribName();
    if ( attribname.isEmpty() )
	attribname = uiAF().attrNameOf( attrtypefld->attr() );

    Desc* newdesc = PF().createDescCopy( attribname );
    if ( !newdesc )
	mErrRetNull( "Cannot create attribdesc" )

    newdesc->setDescSet( attrset );
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

    if ( attrset->isAttribUsed( curdesc->id() ) )
    {
	uiMSG().error( "Cannot remove this attribute. It is used\n"
		       "as input for another attribute" );
	return;
    }

    const int curidx = attrdescs.indexOf( curdesc );
    attrset->removeDesc( attrset->getID(*curdesc) );
    newList( curidx );
    removeNotUsedAttr();
    adsman->setSaved( false );
}


void uiAttribDescSetEd::handleSensitivity()
{
    bool havedescs = !attrdescs.isEmpty();
    rmbut->setSensitive( havedescs );
    revbut->setSensitive( havedescs );
}


bool uiAttribDescSetEd::acceptOK( CallBacker* )
{
    if ( !doCommit() )
	return false;
    removeNotUsedAttr();
    if ( saveButtonChecked() && !doSave(true) )
	return false;
    
    if ( inoutadsman )
        inoutadsman->setSaved( adsman->isSaved() );

    prevsavestate = saveButtonChecked();
    nmprefgrp = attrtypefld->group();
    return true;
}


bool uiAttribDescSetEd::rejectOK( CallBacker* )
{
    setid = cancelsetid;
    return true;
}


void uiAttribDescSetEd::newList( int newcur )
{
    prevdesc = 0;
    updateUserRefs();
    // Fix for continuous call during re-build of list
    updating_fields = true;
    attrlistfld->empty();
    attrlistfld->addItems( userattrnames );
    updating_fields = false;	
    if ( newcur < 0 ) newcur = 0;
    if ( newcur >= attrlistfld->size() ) newcur = attrlistfld->size()-1;
    if ( !userattrnames.isEmpty() )
    {
	attrlistfld->setCurrentItem( newcur );
	prevdesc = curDesc();
    }
    updateFields();
    handleSensitivity();
}


void uiAttribDescSetEd::updateFields( bool set_type )
{
    updating_fields = true;
    updateAttrName();
    Desc* desc = curDesc();
    uiAttrDescEd* curde = 0;
    if ( set_type )
    {
	BufferString typenm = desc ? desc->attribName() : "RefTime";
	attrtypefld->setAttr( uiAF().dispNameOf(typenm) );
	curde = curDescEd();
    }
    if ( !curde )
	curde = curDescEd();

    BufferString attribname = curde ? curde->attribName() : "";
    if ( attribname.isEmpty() )
	attribname = uiAF().attrNameOf( attrtypefld->attr() );
    const bool isrightdesc = desc && attribname == desc->attribName();

    Desc* dummydesc = PF().createDescCopy( attribname );
    if ( !dummydesc )
	dummydesc = new Desc( "Dummy" );
    
    dummydesc->ref();
    dummydesc->setDescSet( attrset );
    const bool is2d = adsman ? adsman->is2D() : attrset->is2D();
    for ( int idx=0; idx<desceds.size(); idx++ )
    {
	uiAttrDescEd* de = desceds[idx];
	if ( !de ) continue;

	if ( curde == de )
	{
	    if ( !isrightdesc )
		dummydesc->ref();
	    de->setDesc( isrightdesc ? desc : dummydesc, adsman );
	}

	de->setDescSet( attrset );
	const bool dodisp = de == curde;
	de->display( dodisp );
    }
    dummydesc->unRef();
    updating_fields = false;
}


bool uiAttribDescSetEd::doCommit( bool useprev )
{
    Desc* usedesc = useprev ? prevdesc : curDesc();
    if ( !usedesc )
	return false;
    
    BufferString newattr = uiAF().attrNameOf( attrtypefld->attr() );
    BufferString oldattr = usedesc->attribName();
    bool checkusrref = true;
    if ( oldattr != newattr )
    {
	BufferString msg = "This will change the type of existing attribute '";
	msg += usedesc->userRef();
	msg += "'.\nThis will remove previous definition of the attribute.\n";
	msg +="If you want to avoid this please use 'Cancel' and 'Add as new'.";
	msg += "\nAre you sure you want to change the attribute type?";
	int res = uiMSG().askGoOn( msg, false );
	if ( res == 1 )
	{
	    checkusrref = false;
	    DescID id = usedesc->id();
	    TypeSet<DescID> attribids;
	    attrset->getIds( attribids );
	    int oldattridx = attribids.indexOf( id );
	    Desc* newdesc = createAttribDesc( checkusrref );
	    if ( !newdesc ) return false;
	    
	    attrset->removeDesc( id );
	    if ( attrset->errMsg() )
	    {
		uiMSG().error( attrset->errMsg() );
		newdesc->unRef();
		return false;
	    }
	    attrset->insertDesc( newdesc, oldattridx, id );
	    const int curidx = attrdescs.indexOf( curDesc() );
	    newList( curidx );
	    removeNotUsedAttr();
	    adsman->setSaved( false );
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
    BufferString selnm( attrlistfld ? attrlistfld->getText() : "" );
    userattrnames.deepErase();
    attrdescs.erase();

    for ( int iattr=0; iattr<attrset->nrDescs(); iattr++ )
    {
	const DescID descid = attrset->getID( iattr );
	Desc* desc = attrset->getDesc( descid );
	if ( !desc || desc->isHidden() || desc->isStored() ) continue;

	attrdescs += desc;
	userattrnames.add( desc->userRef() );
    }

    int newselidx = userattrnames.indexOf( selnm );
    if ( newselidx < 0 ) newselidx = 0;
}


Desc* uiAttribDescSetEd::curDesc() const
{
    int selidx = attrlistfld->currentItem();
    uiAttribDescSetEd* nct = const_cast<uiAttribDescSetEd*>( this );
    return selidx < 0 ? 0 : nct->attrdescs[selidx];
}


uiAttrDescEd* uiAttribDescSetEd::curDescEd()
{
    const char* attrstr = attrtypefld->attr();
    BufferString attrnm = uiAF().attrNameOf( attrstr );
    if ( !attrnm ) attrnm = attrstr;

    uiAttrDescEd* ret = 0;
    for ( int idx=0; idx<desceds.size(); idx++ )
    {
	uiAttrDescEd* de = desceds[idx];
	if ( !de ) continue;

	if ( attrnm == de->attribName() )
	    return de;
    }
    return ret;
}


void uiAttribDescSetEd::updateCurDescEd()
{
    curDescEd()->setDesc( curDesc(), adsman );
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
    attrset->getIds( ids );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const Desc& ad = *attrset->getDesc( ids[idx] );
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
    BufferString usertypednm( attrnmfld->text() );
    char* ptr = usertypednm.buf();
    mTrimBlanks( ptr );
    const BufferString newnm( ptr );

    if ( newnm == attrdesc->userRef() ) return true;
    else if ( !validName(newnm) ) return false;

    const char* res = curDescEd()->commit();
    if ( res )
	{ uiMSG().error( res ); return false; }

    attrdesc->setUserRef( newnm );
    int selidx = userattrnames.indexOf( attrlistfld->getText() );
    newList( selidx );
    return true;
}


void uiAttribDescSetEd::updateAttrName()
{
    Desc* curdesc = curDesc();
    attrnmfld->setText( curdesc ? curdesc->userRef() : "" );
}


bool uiAttribDescSetEd::doSetIO( bool forread )
{
    if ( !setctio.ioobj )
    {
	if ( setid.isEmpty() ) return false;

	setctio.ioobj = IOM().get( setid );
	if ( !setctio.ioobj ) 
	    mErrRetFalse("Cannot find attribute set in data base")
    }

    BufferString bs;
    if ( ( forread && 
	   !AttribDescSetTranslator::retrieve(*attrset,setctio.ioobj,bs) )
    || ( !forread && 
	 !AttribDescSetTranslator::store(*attrset,setctio.ioobj,bs) ) )
	mErrRetFalse(bs)
    if ( !bs.isEmpty() )
	pErrMsg( bs );

    if ( forread )
    {
	adsman->setDescSet( attrset );
	adsman->fillHist();
    }	
    setid = setctio.ioobj->key();
    return true;
}


void uiAttribDescSetEd::newSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    adsman->inputHistory().clear();
    updateFields();

    attrset->removeAll();
    setctio.ioobj = 0;
    setid = -1;
    updateUserRefs();
    newList( -1 );
    attrsetfld->setText( "" );
    adsman->setSaved( true );
}


void uiAttribDescSetEd::openSet( CallBacker* )
{
    if ( !offerSetSave() ) return;
    setctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, setctio );
    if ( dlg.go() && dlg.ioObj() )
	openAttribSet( dlg.ioObj() );

}

void uiAttribDescSetEd::openAttribSet( const IOObj* ioobj )
{
    if ( !ioobj ) return;
    IOObj* oldioobj = setctio.ioobj; setctio.ioobj = 0;
    setctio.setObj( ioobj->clone() );
    if ( !doSetIO( true ) )
	setctio.setObj( oldioobj );
    else
    {
	delete oldioobj;
	setid = setctio.ioobj->key();
	newList( -1 );
	attrsetfld->setText( setctio.ioobj->name() );
	adsman->setSaved( true );
	TypeSet<DescID> ids;
	attrset->getIds( ids );
	for ( int idx=0; idx<attrset->nrDescs(); idx++ )
	{
	    Desc* ad = attrset->getDesc( ids[idx] );
	    if ( !ad ) continue;
	    if ( ad->isStored() && ad->isSatisfied()==2 )
	    {
		BufferString msg = "The attribute: '";
		msg += ad->userRef();
		msg += "' will be removed\n";
		msg += "Storage ID is no longer valid";
		uiMSG().message( msg );
		attrset->removeDesc( ad->id() );
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

    uiSelectFromList::Setup setup( "Default Attribute Sets", attribnames );
    setup.dlgtitle( "Select attribute set" );
    uiSelectFromList dlg( this, setup );
    if ( !dlg.go() ) return;
    
    const int selitm = dlg.selection();
    if ( selitm < 0 ) return;
    const char* filenm = attribfiles[selitm]->buf();

    importFromFile( filenm );
    attrsetfld->setText( attribnames[selitm]->buf() );
}


static void gtDefaultAttribsets( const char* dirnm, bool is2d,
				 BufferStringSet& attribfiles,
				 BufferStringSet& attribnames )
{
    if ( !dirnm || !File_exists(dirnm) )
	return;

    DirList attrdl( dirnm, DirList::DirsOnly, "*Attribs" );
    for ( int idx=0; idx<attrdl.size(); idx++ )
    {
	FilePath fp( dirnm );
	fp.add( attrdl[idx]->buf() ).add( "index" );
	IOPar iopar("AttributeSet Table");
	iopar.read( fp.fullPath(), sKey::Pars, false );
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

	    if ( !File_exists(attrfnm) ) continue;
		
	    attribnames.add( subpar->getKey(idy) );
	    attribfiles.add( attrfnm );
	}
    }
}


void uiAttribDescSetEd::getDefaultAttribsets( BufferStringSet& attribfiles,
					      BufferStringSet& attribnames )
{
    const bool is2d = adsman ? adsman->is2D() : attrset->is2D();
    gtDefaultAttribsets( mGetApplSetupDataDir(), is2d, attribfiles,
	    		 attribnames );
    gtDefaultAttribsets( mGetSWDirDataDir(), is2d, attribfiles, attribnames );
}


void uiAttribDescSetEd::importFromFile( const char* filenm )
{
    IOStream* iostrm = new IOStream( "tmp" );
    iostrm->setGroup( setctio.ctxt.trgroup->userName() );
    iostrm->setTranslator( "dGB" );
    iostrm->setFileName( filenm );
    IOObj* oldioobj = setctio.ioobj ? setctio.ioobj->clone() : 0;
    setctio.setObj( iostrm );
    if ( !doSetIO(true) )
	setctio.setObj( oldioobj );
    else
    {
	delete oldioobj;
	setid = setctio.ioobj->key();
	replaceStoredAttr();
	newList( -1 );
	attrsetfld->setText( "" );
	setctio.ioobj = 0;
    }
}


void uiAttribDescSetEd::importSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    const char* ptr = GetBaseDataDir();
    if ( !ptr ) return;

    PtrMan<DirList> dirlist = new DirList( ptr, DirList::DirsOnly );
    dirlist->sort();

    uiSelectFromList::Setup setup( "Survey", *dirlist );
    setup.dlgtitle( "Select survey" );
    uiSelectFromList dlg( this, setup );
    if ( dlg.go() )
    {
	FilePath fp( ptr ); fp.add( dlg.selFld()->getText() );
	IOM().setRootDir( fp.fullPath() );
	setctio.ctxt.forread = true;
        uiIOObjSelDlg objdlg( this, setctio );
	if ( objdlg.go() && objdlg.ioObj() )
	{
	    IOObj* oldioobj = setctio.ioobj; setctio.ioobj = 0;
	    setctio.setObj( objdlg.ioObj()->clone() );
	    if ( !doSetIO( true ) )
		setctio.setObj( oldioobj );
	    else
	    {
		delete oldioobj;
		setid = setctio.ioobj->key();
		replaceStoredAttr();
		newList( -1 );
		attrsetfld->setText( "" );
		setctio.ioobj = 0;
	    }
	}
    }
    
    IOM().setRootDir( GetDataDir() );
}


void uiAttribDescSetEd::importFile( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiGetFileForAttrSet dlg( this, true, inoutadsman->is2D() );
    if ( dlg.go() )
	importFromFile( dlg.fileName() );
}


void uiAttribDescSetEd::job2Set( CallBacker* )
{
    if ( !offerSetSave() ) return;
    uiGetFileForAttrSet dlg( this, false, inoutadsman->is2D() );
    if ( dlg.go() )
    {
	if ( dlg.attrSet().nrDescs(false,false) < 1 )
	    mErrRet( "No usable attributes in file" )

	IOPar iop; dlg.attrSet().fillPar( iop );
	attrset->removeAll();
	attrset->usePar( iop );
	adsman->setSaved( false );

	setctio.setObj( 0 );
	newList( -1 ); attrsetfld->setText( "" );
    }
}


void uiAttribDescSetEd::crossPlot( CallBacker* )
{
    if ( !adsman || !adsman->descSet() ) return;

    uiAttribCrossPlot* dlg = new uiAttribCrossPlot( 0, *adsman->descSet() );
    dlg->setDeleteOnClose( true );
    dlg->show();
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


bool uiAttribDescSetEd::offerSetSave()
{
    doCommit( true );
    bool saved = adsman->isSaved();
    BufferString msg( "Attribute set is not saved.\nSave now?" );
    if ( !saved && uiMSG().askGoOn( msg ) )
	return doSave(false);
    return true;
}


void uiAttribDescSetEd::changeInput( CallBacker* )
{
    attrlistfld->empty();
    updateFields();
    replaceStoredAttr();
    newList(-1);
    adsman->setSaved( false );
}


void uiAttribDescSetEd::replaceStoredAttr()
{
    uiStoredAttribReplacer replacer( this, *attrset );
    replacer.go();
}


void uiAttribDescSetEd::removeNotUsedAttr()
{
     if ( attrset ) attrset->removeUnused();
}


bool uiAttribDescSetEd::is2D() const
{
    if ( adsman ) 
	return adsman->is2D();
    else if ( attrset ) 
	return attrset->is2D();
    else 
	return false;
}
