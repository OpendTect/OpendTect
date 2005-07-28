/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.cc,v 1.2 2005-07-28 10:53:50 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdescseted.h"
#include "uiattrdesced.h"
#include "uiattrfact.h"
#include "attribfactory.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsettr.h"
#include "attribparam.h"
#include "uiattrinpdlg.h"
#include "attribsel.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "dirlist.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iostrm.h"
#include "iopar.h"
#include "iodir.h"
#include "ptrman.h"
#include "pixmap.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilineedit.h"
#include "uilistboxdlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "seistype.h"
#include "survinfo.h"
#include "bufstringset.h"
#include "uifileinput.h"


extern "C" const char* GetBaseDataDir();
static bool prevsavestate = true;
static bool evaldlgpoppedup = false;

using namespace Attrib;

uiAttribDescSetEd::uiAttribDescSetEd( uiParent* p, DescSetMan* adsm )
    : uiDialog(p,uiDialog::Setup("Attribute Set","","101.0.0")
	.savebutton().savetext("Save on OK  ")
	.menubar(true)
	.modal(false))
    , inoutadsman(adsm)
    , userattrnames(*new BufferStringSet)
    , setctio(*mMkCtxtIOObj(AttribDescSet))
    , prevdesc(0)
    , attrset(0)
    , dirshowcb(this)
    , evalattrcb(this)
{
    createMenuBar();
    createToolBar();
    createGroups();

    init();
}


#define mInsertItem( txt, func ) \
    filemnu->insertItem( new uiMenuItem(txt,mCB(this,uiAttribDescSetEd,func)) )

void uiAttribDescSetEd::createMenuBar()
{
    uiMenuBar* menu = menuBar();
    if( !menu )		{ pErrMsg("huh?"); return; }

    uiPopupMenu* filemnu = new uiPopupMenu( this, "File" );
    mInsertItem( "&New set ...", newSet );
    mInsertItem( "&Open set ...", openSet );
    mInsertItem( "&Save set ...", savePush );
    mInsertItem( "&Change input ...", changeInput );
    filemnu->insertSeparator();
    mInsertItem( "Open &Default set ...", defaultSet );
    mInsertItem( "&Import set ...", importSet );
    mInsertItem( "Import set from &file ...", importFile );

    menu->insertItem( filemnu );
}


#define mAddButton(pm,func,tip) \
    toolbar->addButton( ioPixmap( GetDataFileName(pm) ), \
	    		mCB(this,uiAttribDescSetEd,func), tip )

void uiAttribDescSetEd::createToolBar()
{
    toolbar = new uiToolBar( this, "AttributeSet tools" );
    mAddButton( "newset.png", newSet, "New attribute set" );
    mAddButton( "openset.png", openSet, "Open attribute set" );
    mAddButton( "saveset.png", savePush, "Save attribute set" );
    mAddButton( "defset.png", defaultSet, "Open default attribute set" );
    mAddButton( "impset.png", importSet, 
	    	"Import attribute set from other survey" );
    toolbar->addSeparator();
    mAddButton( "showattrnow.png", directShow, 
	    	"Redisplay element with current attribute");
    mAddButton( "evalattr.png", evalAttribute, "Evaluate attribute" );
    toolbar->display();
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

    rmbut = new uiPushButton( leftgrp, "Remove selected" );
    rmbut->attach( centeredBelow, attrlistfld );
    rmbut->activated.notify( mCB(this,uiAttribDescSetEd,rmPush) );

    uiSeparator* vertsep = new uiSeparator( this, "Big vert sep", false );
    vertsep->attach( rightOf, leftgrp );
    vertsep->attach( heightSameAs, leftgrp );

//  Right part
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 0, 1 );
    rightgrp->attach( rightOf, vertsep );

    uiGroup* degrp = new uiGroup( rightgrp, "DescEdGroup" );
    degrp->setStretch( 1, 1 );

    attrtypefld = new uiLabeledComboBox( degrp, "Attribute type" );
    attrtypefld->box()->setCurrentItem( 0 );
    attrtypefld->box()->selectionChanged.notify(
			mCB(this,uiAttribDescSetEd,attrTypSel) );

    desceds.allowNull();
    BufferStringSet attrnms;
    for ( int iattr=0; iattr<uiAttribFactory::nrNames(); iattr++ )
	attrnms.add( uiAttribFactory::name(iattr) );

    attrnms.sort();
    for ( int idx=0; idx<attrnms.size(); idx++ )
    {
	const char* attrnm = attrnms.get( idx );
	uiAttrDescEd* de = uiAttribFactory::create( degrp, attrnm );
	desceds += de;
	de->attach( alignedBelow, attrtypefld );
	if ( (SI().zIsTime() && de->useIfZIsTime()) || 
	     (!SI().zIsTime() && de->useIfZIsDepth()) )
	    attrtypefld->box()->addItem( attrnm );

    }
    attrnms.deepErase();

    degrp->setHAlignObj( attrtypefld );

    attrnmfld = new uiLineEdit( rightgrp );
    attrnmfld->setHSzPol( uiObject::medmax );
    attrnmfld->attach( alignedBelow, degrp );
    attrnmfld->returnPressed.notify( mCB(this,uiAttribDescSetEd,addPush) );
    uiLabel* lbl = new uiLabel( rightgrp, "Attribute name" );
    lbl->attach( leftOf, attrnmfld );

    addbut = new uiPushButton( rightgrp, "Add as new" );
    addbut->attach( alignedBelow, attrnmfld );
    addbut->activated.notify( mCB(this,uiAttribDescSetEd,addPush) );

    revbut = new uiPushButton( rightgrp, "Revert changes" );
    revbut->attach( rightTo, addbut );
    revbut->attach( rightBorder );
    revbut->activated.notify( mCB(this,uiAttribDescSetEd,revPush) );
}


void uiAttribDescSetEd::init()
{
    delete attrset;
    attrset = inoutadsman->descSet()->clone();
    adsman = new DescSetMan( attrset );
    adsman->fillHist();
    adsman->setSaved( inoutadsman->isSaved() );

    setid = inoutadsman->attrsetid_;
    IOM().to( setctio.ctxt.stdSelKey() );
    setctio.ioobj = IOM().get( setid );
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
}



#define mErrRetFalse(s) { uiMSG().error( s ); return false; }
#define mErrRet(s) { uiMSG().error( s ); return; }


void uiAttribDescSetEd::attrTypSel( CallBacker* )
{
    updateFields( false );
}


void uiAttribDescSetEd::selChg( CallBacker* )
{
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


void uiAttribDescSetEd::revPush( CallBacker* )
{
    updateFields();
}


void uiAttribDescSetEd::addPush( CallBacker* )
{
    BufferString newnmbs( attrnmfld->text() );
    char* newnm = newnmbs.buf();
    skipLeadingBlanks( newnm );
    removeTrailingBlanks( newnm );
    if ( !validName(newnm) ) return;

    uiAttrDescEd* curde = 0;
    if ( !(curde = curDescEd()) )
	mErrRet( "Cannot add without a valid attribute type" )

    const char* attrstr = attrtypefld->box()->text();
    const char* attribname = uiAttribFactory::defNameForName( attrstr );

    Desc* newdesc = PF().createDescCopy( attribname );
    newdesc->setDescSet( attrset );
    newdesc->ref();
    const char* res = curde->commit( newdesc );
    if ( res )
	mErrRet( res )
    newdesc->setUserRef( newnm );

    if ( attrset->addDesc(newdesc) < 0 )
	{ uiMSG().error( attrset->errMsg() ); newdesc->unRef(); return; }

    newList( attrdescs.size() );
    adsman->setSaved( false );
}


void uiAttribDescSetEd::rmPush( CallBacker* )
{
    Desc* curdesc = curDesc();
    if ( !curdesc ) return;

    attrset->removeDesc( attrset->getID(*curdesc) );
    newList(0);
    removeNotUsedAttr();
    adsman->setSaved( false );
}


void uiAttribDescSetEd::handleSensitivity()
{
    bool havedescs = attrdescs.size() > 0;
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
    
// TODO: check if these lines are necessary
//    if ( !saveButtonChecked() && attrset->isSatisfied() )
//	mErrRetFalse( attrset->errMsg() );

    if ( inoutadsman )
        inoutadsman->setSaved( adsman->isSaved() );

    prevsavestate = saveButtonChecked();
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
    attrlistfld->empty();
    attrlistfld->addItems( userattrnames );
    if ( newcur < 0 ) newcur = 0;
    if ( userattrnames.size() > 0 )
    {
	attrlistfld->setCurrentItem( newcur );
	prevdesc = curDesc();
    }
    updateFields();
    handleSensitivity();
}


void uiAttribDescSetEd::updateFields( bool set_type )
{
    updateAttrName();
    Desc* desc = curDesc();
    uiAttrDescEd* curde = 0;
    if ( set_type )
    {
	if ( !desc )
	{
	    const char* nm =
		uiAttribFactory::nameForDefName( "RefTime" );
	    attrtypefld->box()->setCurrentItem( nm );
	    curde = curDescEd();
	}
	else
	{
	    BufferString typenm = desc->attribName();
	    const char* nm =
		uiAttribFactory::nameForDefName( getAttrTypeName( typenm ) );
	    if ( nm )
		attrtypefld->box()->setCurrentItem( nm );
	    curde = curDescEd();
	}
    }
    if ( !curde )
	curde = curDescEd();

    Desc* dummydesc = new Desc( "Dummy" ); dummydesc->ref();
    dummydesc->setDescSet( attrset );
    for ( int idx=0; idx<desceds.size(); idx++ )
    {
	uiAttrDescEd* de = desceds[idx];
	if ( !de ) continue;

	de->setDesc( desc ? desc : dummydesc, adsman );
	bool dodisp = de == curde;
	if ( dodisp )
	    de->set2D( attrset->is2D() );
	de->display( dodisp );
    }
    dummydesc->unRef();
}


const char* uiAttribDescSetEd::getAttrTypeName( const char* nm )
{
    const char* convdipnm = "3DFilter";

    if ( !strcmp(nm,"Convolve") || !strcmp(nm,"DipFilter") )
	return convdipnm;
    
    return nm;
}


bool uiAttribDescSetEd::doCommit( bool useprev )
{
    Desc* usedesc = useprev ? prevdesc : curDesc();
    if ( !usedesc )
	return false;
    
    const char* attrstr = attrtypefld->box()->text();
    const char* newattr = uiAttribFactory::defNameForName( attrstr );
    BufferString type = usedesc->attribName();
    const char* oldattr = getAttrTypeName( type );
    if ( strcmp(newattr,oldattr) && 
	 !uiMSG().askGoOn("This action will change the attribute type.\n"
			  "Do you want to continue?") )
	return false;

    if ( !setUserRef(usedesc) ) return false;

    uiAttrDescEd* curdesced = curDescEd();
    if ( !curdesced ) return false;

    const char* res = curdesced->commit();
    if ( res )
	mErrRetFalse( res )

    return true;
}


void uiAttribDescSetEd::updateUserRefs()
{
    BufferString selnm;
    if ( attrlistfld )
	selnm = attrlistfld->getText();
    userattrnames.deepErase();
    attrdescs.erase();

    const int nrattrs = attrset->nrDescs();
    for ( int iattr=0; iattr<nrattrs; iattr++ )
    {
	Desc* desc = attrset->getDesc( iattr );
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
    const char* attrstr = attrtypefld->box()->text();
    const char* attrnm = uiAttribFactory::defNameForName( attrstr );
    if ( !attrnm ) attrnm = attrstr;

    uiAttrDescEd* ret = 0;
    for ( int idx=0; idx<desceds.size(); idx++ )
    {
	uiAttrDescEd* de = desceds[idx];
	if ( !de ) continue;

	if ( de->shouldEdit(attrnm) )
	    return de;
	if ( !ret && de->canEdit(attrnm) )
	    ret = de;
    }
    return ret;
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

    TypeSet<int> ids;
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
    BufferString newnm( attrnmfld->text() );

    while ( newnm[(int)(strlen(newnm)-1)] == ' ' )
	newnm[(int)(strlen(newnm)-1)] = 0;

    if ( !strcmp(newnm,attrdesc->userRef()) ) return true;
    else if ( !validName(newnm) ) return false;

    const char* res = curDescEd()->commit();
    if ( res ) uiMSG().error( res );

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
	if ( setid == "" ) return false;

	setctio.ioobj = IOM().get( setid );
	if ( !setctio.ioobj ) 
	    mErrRetFalse("Cannot find attribute set in data base")
    }

    BufferString bs;
    if ( forread && 
	 !AttribDescSetTranslator::retrieve(*attrset,setctio.ioobj,bs)
    ||  !forread && 
	!AttribDescSetTranslator::store(*attrset,setctio.ioobj,bs) )
	mErrRetFalse(bs)
    if ( bs != "" )
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
    {
	IOObj* oldioobj = setctio.ioobj; setctio.ioobj = 0;
        setctio.setObj( dlg.ioObj()->clone() );
	if ( !doSetIO( true ) )
	    setctio.setObj( oldioobj );
	else
	{
	    delete oldioobj;
	    setid = setctio.ioobj->key();
	    newList( -1 );
	    attrsetfld->setText( setctio.ioobj->name() );
	    adsman->setSaved( true );
	    for ( int idx=0; idx<attrset->nrDescs(); idx++ )
	    {
		Desc* ad = attrset->getDesc( idx );
		if ( !ad ) continue;
		if ( ad->isStored() && ad->isSatisfied()==2 )
		{
		    BufferString msg = "This attribute will be removed";
		    uiMSG().message( msg );
		    attrset->removeDesc( ad->id() );
		    idx--;
		}
	    }
	}
    }
}


void uiAttribDescSetEd::defaultSet( CallBacker* )
{
    if ( !offerSetSave() ) return;

    const char* ptr = GetDataFileName(0);
    DirList attrdl( ptr, DirList::DirsOnly, "*Attribs" );

    BufferStringSet attribfiles;
    BufferStringSet attribnames;
    for ( int idx=0; idx<attrdl.size(); idx++ )
    {
	FilePath fp( ptr );
	fp.add( attrdl[idx]->buf() ).add( "index" );
	IOPar iopar("AttributeSet Table");
	iopar.read( fp.fullPath() );
	for ( int idy=0; idy<iopar.size(); idy++ )
	{
	    BufferString attrfnm = iopar.getKey(idy);
	    if ( !FilePath(attrfnm).isAbsolute() )
	    {
		fp.setFileName(attrfnm);
		attrfnm = fp.fullPath();
	    }

	    if ( !File_exists(attrfnm) ) continue;
		
	    attribnames.add( iopar.getValue(idy) );
	    attribfiles.add( attrfnm );
	}
    }

    uiListBoxDlg dlg( this, attribnames, "Default Attribute Sets" );
    dlg.setTitleText( "Select attribute set" );
    if ( !dlg.go() ) return;

    const int selitm = dlg.box()->currentItem();
    const char* filenm = attribfiles[selitm]->buf();

    importFromFile( filenm );
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

    uiListBoxDlg dlg( this, *dirlist, "Survey" );
    dlg.setTitleText( "Select survey" );
    if ( dlg.go() )
    {
	FilePath fp( ptr ); fp.add( dlg.box()->getText() );
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


class uiFileInpDlg : public uiDialog
{
public:
uiFileInpDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Attribute Set","Select filename",0))
{
    fileinput = new uiFileInput( this, "Select AttributeSet file" );
    fileinput->setFilter( "AttributeSet files (*.attr);;All files (*)" );
    fileinput->setDefaultSelectionDir(
			    IOObjContext::getDataDirName(IOObjContext::Attr) );
}

const char* fileName() const
{ return fileinput->fileName(); }

bool acceptOK( CallBacker* )
{
    if ( !fileName() || !*fileName() )
    {
	uiMSG().error( "Please select filename" );
	return false;
    }
    return true;
}

    uiFileInput*	fileinput;
};


void uiAttribDescSetEd::importFile( CallBacker* )
{
    if ( !offerSetSave() ) return;

    uiFileInpDlg dlg( this );
    if ( dlg.go() )
	importFromFile( dlg.fileName() );
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
    TypeSet<int> idset;
    BufferStringSet defstrs;
    TypeSet<int> attrids;
    attrset->getIds( attrids );
    for ( int idx=0; idx<attrids.size(); idx++ )
    {
        Desc* ad = attrset->getDesc( attrids[idx] );
        if ( !ad ) continue;
	BufferString defstr;
	ad->getDefStr( defstr );
        if ( ad->isStored() && defstrs.indexOf(defstr)<0 )
        {
            idset += attrids[idx];
	    defstrs.add( defstr );
        }
    }

    BufferStringSet usrrefs;
    BufferStringSet notused;
    bool found2d = false;
    for ( int idnr=0; idnr<idset.size(); idnr++ )
    {
	usrrefs.erase();
	int crldipid = -1;
	TypeSet<int> attrids;
	attrset->getIds( attrids );
	for ( int idx=0; idx<attrids.size(); idx++ )
        {
            Desc* ad = attrset->getDesc( attrids[idx] );
            if ( !ad || ad->isStored() || ad->isHidden() ) continue;
            for ( int inpnr=0; inpnr<ad->nrInputs(); inpnr++ )
            {
                if ( ad->inputId( inpnr ) == idset[idnr] )
                {
                    usrrefs += new BufferString(ad->userRef());
                }
            }
        }

	Desc* ad = attrset->getDesc( attrset->getID(idset[idnr]));
	
	if ( !usrrefs.size() )
	{
	    BufferString bfstr;
	    ad->getDefStr(bfstr);
	    notused.add(bfstr.buf());
	    continue;
	}

	const bool issteer = ad->dataType() == Seis::Dip;
        uiAttrInpDlg dlg( this, usrrefs, issteer );
	dlg.set2DPol( !idnr ? Both2DAnd3D : (found2d ? Only2D : No2D) ); 
        if ( dlg.go() )
        {
            ad->changeStoredID( dlg.getDefStr() );
            ad->setUserRef( dlg.getUserRef() );
	    if ( !found2d && dlg.is2D() )
		found2d = true;
	}
    }

    for ( int idx=0; idx<notused.size(); idx++ )
    {
	while( true )
	{
	    const int dscnr = attrset->getID( notused[0]->buf(), false );
	    if ( dscnr < 0 ) break;
	    attrset->removeDesc( dscnr );
	}
    }

    attrset->removeUnused( true );
}


void uiAttribDescSetEd::removeNotUsedAttr()
{
     if ( attrset ) attrset->removeUnused();
}
