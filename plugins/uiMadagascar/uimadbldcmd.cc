/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadbldcmd.cc,v 1.8 2007-12-11 15:18:26 cvsbert Exp $";

#include "uimadbldcmd.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uisplitter.h"
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
	: uiGroup(p,"Madagascar command builder")
	, cmdAvailable(this)
    	, cmdisadd_(true)
{
    bool allok = ODMad::PI().errMsg().isEmpty();
    if ( allok && !ODMad::PI().scanned() )
    {
	Executor* ex = ODMad::PI().getScanner();
	uiExecutor dlg( this, *ex );
	allok = dlg.go() && ODMad::PI().defs().size() > 0;
	delete ex;
    }

    uiGroup* selgrp = new uiGroup( this, "Sel grp" );
    if ( allok )
	createMainPart( selgrp );
    else
	new uiLabel( selgrp, ODMad::PI().errMsg() );
    setHAlignObj( selgrp );

    uiSeparator* hsep = new uiSeparator( this, "Hor sep" );
    hsep->setStretch( 2, 0 );
    hsep->attach( ensureBelow, selgrp );

    uiGroup* lowgrp = createLowGroup();
    lowgrp->attach( alignedBelow, selgrp );
    lowgrp->attach( ensureBelow, hsep );
}


void uiMadagascarBldCmd::createMainPart( uiGroup* selgrp )
{
    uiGroup* lsgrp = new uiGroup( selgrp, "Left Selection group" );
    selgrp->setHAlignObj( lsgrp );

    BufferStringSet grps; grps.add( "All" ); grps.add( "Search results" );
    BufferStringSet madgrps( ODMad::PI().groups() );
    madgrps.sort(); grps.add( madgrps, false );
    uiLabeledComboBox* groupfld = new uiLabeledComboBox( lsgrp, grps, "Group" );
    groupfld_ = groupfld->box();
    groupfld_->setToolTip( "Madagascar program group" );
    groupfld_->selectionChanged.notify( mCB(this,uiMadagascarBldCmd,groupChg) );
    uiLabeledListBox* progfld = new uiLabeledListBox( lsgrp, "Program" );
    progfld->attach( alignedBelow, groupfld );
    progfld_ = progfld->box();
    progfld_->selectionChanged.notify( mCB(this,uiMadagascarBldCmd,progChg) );
    progfld_->doubleClicked.notify( mCB(this,uiMadagascarBldCmd,dClick) );
    progfld_->setPrefHeightInChar( 12 );
    lsgrp->setHAlignObj( groupfld );

    uiGroup* infogrp = new uiGroup( selgrp, "Right Selection group" );

    srchfld_ = new uiLineEdit( infogrp, "", "Search field" );
    srchfld_->setToolTip( "Search expression" );
    srchfld_->setPrefWidthInChar( 15 );
    srchfld_->returnPressed.notify( mCB(this,uiMadagascarBldCmd,doSearch) );
    uiPushButton* srchbut = new uiPushButton( infogrp, "<< &Find expression",
	    				      true );
    srchbut->activated.notify( mCB(this,uiMadagascarBldCmd,doSearch) );
    srchbut->attach( rightOf, srchfld_ );

    descfld_ = new uiLineEdit( infogrp, "", "Desc fld" );
    descfld_->attach( alignedBelow, srchfld_ );
    descfld_->setReadOnly( true );
    descfld_->setStretch( 0, 2 );
    commentfld_ = new uiTextEdit( infogrp, "Comments", true );
    commentfld_->attach( alignedBelow, descfld_ );
    commentfld_->setStretch( 1, 1 );
    commentfld_->setPrefHeightInChar( 14 );
    commentfld_->setPrefWidthInChar( 40 );

    infogrp->setHAlignObj( srchbut );

    uiSplitter* vspl = new uiSplitter( selgrp, "vert spl", true );
    vspl->addGroup( lsgrp ); vspl->addGroup( infogrp );
}


uiGroup* uiMadagascarBldCmd::createLowGroup()
{
    uiGroup* lowgrp = new uiGroup( this, "Low grp" );

    synopsfld_ = new uiLineEdit( lowgrp, "", "Synopsis edit line" );
    synopsfld_->setReadOnly( true );
    synopsfld_->setStretch( 0, 2 );
    new uiLabel( lowgrp, "Synopsis", synopsfld_ );

    cmdfld_ = new uiLineEdit( lowgrp, "", "Command line edit line" );
    cmdfld_->attach( alignedBelow, synopsfld_ );
    cmdfld_->setStretch( 0, 2 );
    new uiLabel( lowgrp, "Command line", cmdfld_ );

    uiPushButton* edbut = new uiPushButton( lowgrp, "&Replace", true );
    edbut->setToolTip( "Replace current command" );
    edbut->activated.notify( mCB(this,uiMadagascarBldCmd,doEdit) );
    edbut->attach( rightOf, cmdfld_ );
    uiPushButton* addbut = new uiPushButton( lowgrp, "&Add", true );
    addbut->setToolTip( "Add to process flow" );
    addbut->activated.notify( mCB(this,uiMadagascarBldCmd,doAdd) );
    addbut->attach( rightTo, edbut );
    addbut->attach( rightBorder );

    lowgrp->setHAlignObj( synopsfld_ );
    return lowgrp;
}


uiMadagascarBldCmd::~uiMadagascarBldCmd()
{
}


const char*  uiMadagascarBldCmd::command() const
{
    return cmdfld_->text();
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
    progfld_->empty();
    if ( defs.size() < 1 ) return;

    for ( int idx=0; idx<defs.size(); idx++ )
	progfld_->addItem( defs[idx]->name_ );

    progfld_->setCurrentItem( 0 );
    progChg( 0 );
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


#define mImplButFn(isadd) \
    if ( !*cmdfld_->text() ) return; \
    cmdisadd_ = isadd; \
    cmdAvailable.trigger()

void uiMadagascarBldCmd::doAdd( CallBacker* )	{ mImplButFn( true ); }
void uiMadagascarBldCmd::doEdit( CallBacker* )	{ mImplButFn( false ); }
