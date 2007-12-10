/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadbldcmd.cc,v 1.7 2007-12-10 17:24:07 cvsbert Exp $";

#include "uimadbldcmd.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uiexecutor.h"
#include "uitextedit.h"
#include "maddefs.h"
#include "executor.h"


static BufferString& separateProgName( const char* cmd, bool wantprog )
{
    static BufferString ret;
    ret = cmd; // resize to fit

    while ( *cmd && isspace(*cmd) ) cmd++;
    char* retptr = ret.buf();
    *retptr = '\0';
    if ( wantprog )
	while ( *cmd && !isspace(*cmd) ) *retptr++ = *cmd++; 
    else
    {
	while ( *cmd && !isspace(*cmd) ) cmd++; 
	while ( *cmd && isspace(*cmd) ) cmd++; 
	while ( *cmd ) *retptr++ = *cmd++; 
    }
    *retptr = '\0';
    return ret;
}


uiMadagascarBldCmd::uiMadagascarBldCmd( uiParent* p )
	: uiDialog( p, Setup( "Madagascar command building",
			      "Build Madagascar command",
			      "0.0.0").modal(false) )
	, cmdAvailable(this)
	, hideReq(this)
    	, cmdisnew_(true)
{
    setCtrlStyle( uiDialog::LeaveOnly );
    bool allok = ODMad::PI().errMsg().isEmpty();
    if ( allok && !ODMad::PI().scanned() )
    {
	Executor* ex = ODMad::PI().getScanner();
	uiExecutor dlg( this, *ex );
	allok = dlg.go() && ODMad::PI().defs().size() > 0;
	delete ex;
    }

    uiSeparator* sep;
    if ( allok )
	sep = createMainPart();
    else
    {
	uiLabel* lbl = new uiLabel( this, ODMad::PI().errMsg() );
	sep = new uiSeparator( this, "low sep" );
	sep->attach( stretchedBelow, lbl );
    }

    cmdfld_ = new uiLineEdit( this, "", "Command line edit line" );
    if ( allok )
	cmdfld_->attach( alignedBelow, synopsfld_ );
    else
	cmdfld_->attach( ensureBelow, sep );
    cmdfld_->setStretch( 0, 2 );
    cmdfld_->setHSzPol( uiObject::WideMax );
    new uiLabel( this, "Command line", cmdfld_ );

    uiPushButton* edbut = new uiPushButton( this, "&Replace", true );
    edbut->setToolTip( "Replace current command" );
    edbut->activated.notify( mCB(this,uiMadagascarBldCmd,doEdit) );
    edbut->attach( rightOf, cmdfld_ );
    uiPushButton* addbut = new uiPushButton( this, "&Add", true );
    addbut->setToolTip( "Add to process flow" );
    addbut->activated.notify( mCB(this,uiMadagascarBldCmd,doAdd) );
    addbut->attach( rightTo, edbut );
    addbut->attach( rightBorder );
}


uiMadagascarBldCmd::~uiMadagascarBldCmd()
{
}


const char*  uiMadagascarBldCmd::command() const
{
    return cmdfld_->text();
}


uiSeparator* uiMadagascarBldCmd::createMainPart()
{
    uiGroup* selgrp = new uiGroup( this, "Selection group" );
    uiGroup* lsgrp = new uiGroup( selgrp, "Left Selection group" );
    selgrp->setHAlignObj( lsgrp );

    BufferStringSet grps( ODMad::PI().groups() );
    grps.sort();
    grps.insertAt( new BufferString("All"), 0 );
    uiLabeledComboBox* groupfld = new uiLabeledComboBox( lsgrp, grps,"Group" );
    groupfld_ = groupfld->box();
    groupfld_->selectionChanged.notify( mCB(this,uiMadagascarBldCmd,groupChg) );
    groupfld_->setStretch( 0, 0 );
    lsgrp->setHAlignObj( groupfld );
    uiLabeledListBox* progfld = new uiLabeledListBox( lsgrp, "Program" );
    progfld->attach( alignedBelow, groupfld );
    progfld_ = progfld->box();
    progfld_->selectionChanged.notify( mCB(this,uiMadagascarBldCmd,progChg) );
    progfld_->doubleClicked.notify( mCB(this,uiMadagascarBldCmd,dClick) );
    progfld_->setStretch( 0, 1 );
    progfld_->setPrefHeightInChar( 16 );

    uiSeparator* vsep = new uiSeparator( selgrp, "vert sep", false );
    vsep->attach( rightOf, lsgrp );
    vsep->setStretch( 0, 2 );
    descfld_ = new uiLineEdit( selgrp, "", "Desc fld" );
    descfld_->attach( rightOf, vsep );
    descfld_->setReadOnly( true );
    descfld_->setPrefWidthInChar( 50 );
    descfld_->setStretch( 0, 2 );
    descfld_->setHSzPol( uiObject::WideMax );
    commentfld_ = new uiTextEdit( selgrp, "Comments", true );
    commentfld_->attach( alignedBelow, descfld_ );
    commentfld_->setStretch( 2, 1 );
    // commentfld_->setPrefWidthInChar( 50 );
    commentfld_->setPrefHeightInChar( 15 );

    srchfld_ = new uiLineEdit( selgrp, "", "Search field" );
    srchfld_->attach( alignedBelow, commentfld_ );
    srchfld_->returnPressed.notify( mCB(this,uiMadagascarBldCmd,doSearch) );
    uiPushButton* srchbut = new uiPushButton( selgrp, "&Search >>", true );
    srchbut->activated.notify( mCB(this,uiMadagascarBldCmd,doSearch) );
    srchbut->attach( rightOf, srchfld_ );
    srchresfld_ = new uiComboBox( selgrp, "Search results" );
    srchresfld_->setToolTip( "Programs matching search" );
    srchresfld_->selectionChanged.notify(
	    		mCB(this,uiMadagascarBldCmd,searchBoxSel) );
    srchresfld_->attach( rightTo, srchbut );
    srchresfld_->attach( rightBorder );

    uiSeparator* sep = new uiSeparator( this, "low sep" );
    sep->attach( stretchedBelow, selgrp );

    synopsfld_ = new uiLineEdit( this, "", "Synopsis edit line" );
    synopsfld_->attach( alignedBelow, selgrp );
    synopsfld_->attach( ensureBelow, sep );
    synopsfld_->setReadOnly( true );
    synopsfld_->setStretch( 0, 2 );
    synopsfld_->setHSzPol( uiObject::WideMax );
    new uiLabel( this, "Synopsis", synopsfld_ );
    return sep;
}


void uiMadagascarBldCmd::setCmd( const char* cmd )
{
    setProgName( separateProgName( cmd, true ) );
    cmdfld_->setText( cmd );
}


void uiMadagascarBldCmd::setProgName( const char* pnm )
{
    const BufferString prognm( pnm );
    if ( !prognm.isEmpty() )
    {
	const ODMad::ProgDef* def = getDef( prognm );
	if ( def )
	{
	    setGroupProgs( def->group_ );
	    if ( def->group_ )
		groupfld_->setCurrentItem( def->group_->buf() );
	}
    }
    progfld_->setCurrentItem( prognm );
    groupChg( 0 );
}


void uiMadagascarBldCmd::setGroupProgs( const BufferString* curgrp )
{
    progfld_->empty();
    const ObjectSet<ODMad::ProgDef>& defs = ODMad::PI().defs();
    for ( int idx=0; idx<defs.size(); idx++ )
    {
	const ODMad::ProgDef& def = *defs[idx];
	if ( !curgrp || def.group_ == curgrp )
	    progfld_->addItem( def.name_ );
    }
}


void uiMadagascarBldCmd::doSearch( CallBacker* )
{
    const BufferString srchkey( srchfld_->text() );
    if ( srchkey.isEmpty() ) return;

    ObjectSet<const ODMad::ProgDef> defs;
    ODMad::PI().search( srchkey, defs );
    srchresfld_->empty();
    if ( defs.size() < 1 ) return;

    for ( int idx=0; idx<defs.size(); idx++ )
	srchresfld_->addItem( defs[idx]->name_ );

    srchresfld_->setCurrentItem( 0 );
    searchBoxSel( srchresfld_ );
}


void uiMadagascarBldCmd::searchBoxSel( CallBacker* )
{
    if ( srchresfld_->size() < 1 ) return;
    setProgName( srchresfld_->text() );
}


void uiMadagascarBldCmd::groupChg( CallBacker* c )
{
    const BufferString prognm = progfld_->getText();
    const BufferString* curgrp = find( ODMad::PI().groups(), groupfld_->text());
    setGroupProgs( curgrp );

    if ( !progfld_->isPresent(prognm) )
	setInput( 0 );
    else
    {
	setInput( getDef(prognm) );
	progfld_->setCurrentItem( prognm );
    }
}


const ODMad::ProgDef* uiMadagascarBldCmd::getDef( const char* prognm )
{
    const ObjectSet<ODMad::ProgDef>& defs = ODMad::PI().defs();
    for ( int idx=0; idx<defs.size(); idx++ )
    {
	const ODMad::ProgDef& def = *defs[idx];
	if ( def.name_ == prognm )
	    return &def;
    }
    return 0;
}

void uiMadagascarBldCmd::dClick( CallBacker* c )
{
    BufferString txt( progfld_->getText() ); txt += " ";
    cmdfld_->setText( txt );
}


void uiMadagascarBldCmd::progChg( CallBacker* )
{
    setInput( getDef( progfld_->getText() ) );
}


void uiMadagascarBldCmd::setInput( const ODMad::ProgDef* def )
{
    descfld_->setText( def ? def->shortdesc_.buf() : "" );
    commentfld_->setText( def ? def->comment_.buf() : "" );
    synopsfld_->setText( def ? def->synopsis_.buf() : "" );
    if ( !def ) return;

    BufferString cmd = cmdfld_->text();
    if ( separateProgName(cmdfld_->text(),false).isEmpty() )
    {
	BufferString txt( def->name_ ); txt += " ";
	cmdfld_->setText( txt );
    }
}


#define mImplButFn(isnw) \
    if ( !cmdOK() ) return; \
    cmdisnew_ = isnw; \
    cmdAvailable.trigger()

void uiMadagascarBldCmd::doAdd( CallBacker* )	{ mImplButFn( true ); }
void uiMadagascarBldCmd::doEdit( CallBacker* )	{ mImplButFn( false ); }


bool uiMadagascarBldCmd::cmdOK()
{
    BufferString newcmd = cmdfld_->text();
    if ( newcmd.isEmpty() )
    {
	uiMSG().error( "Please specify a command" );
	return false;
    }
    return true;
}


bool uiMadagascarBldCmd::rejectOK( CallBacker* )
{
    hideReq.trigger();
    return false;
}
