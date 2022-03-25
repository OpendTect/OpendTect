/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/


#include "uimadbldcmd.h"
#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uicompoundparsel.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uitextedit.h"

#include "executor.h"
#include "maddefs.h"
#include "madproc.h"
#include "perthreadrepos.h"

static const char* sKeyAll = "All";
static const char* sKeySrchRes = "Search results";

static BufferString& separateProgName( const char* cmd, bool wantprog )
{
    mDeclStaticString( ret );
    ret = cmd; // resize to fit

    mSkipBlanks( cmd );
    char* retptr = ret.getCStr();
    *retptr = '\0';
    if ( wantprog )
	while ( *cmd && !iswspace(*cmd) ) *retptr++ = *cmd++;
    else
    {
	mSkipNonBlanks( cmd );
	mSkipBlanks( cmd );
	while ( *cmd ) *retptr++ = *cmd++;
    }
    *retptr = '\0';
    return ret;
}


class uiMadagascarBldPlotCmd : public uiCompoundParSel
{ mODTextTranslationClass(uiMadagascarBldPlotCmd);
public:

			uiMadagascarBldPlotCmd(uiParent*);
			~uiMadagascarBldPlotCmd();

    BufferString	getPlotCommand() const		{ return getSummary(); }
    void		setPlotCmd(const char*);

protected:

    BufferString	createplotcmd_;
    BufferString	viewplotcmd_;

    BufferStringSet&	cmdlist_;

    BufferString	getSummary() const;
    void		doDlg(CallBacker*);
};


uiMadagascarBldPlotCmd::uiMadagascarBldPlotCmd( uiParent* p )
  : uiCompoundParSel(p,tr("Plot Command"),uiStrings::sCreate())
  , cmdlist_(*new BufferStringSet)
{
    const ObjectSet<ODMad::ProgDef>& defs = ODMad::PI().defs();
    for ( int idx=0; idx<defs.size(); idx++ )
    {
	const ODMad::ProgDef& def = *defs[idx];
	if ( def.group_->startsWith("plot",CaseInsensitive)
	  && def.synopsis_.contains("in.rsf") )
	    cmdlist_.add( def.name_ );
    }

    butPush.notify( mCB(this,uiMadagascarBldPlotCmd,doDlg) );
}


uiMadagascarBldPlotCmd::~uiMadagascarBldPlotCmd()
{
    delete &cmdlist_;
}


void uiMadagascarBldPlotCmd::setPlotCmd( const char* cmd )
{
    createplotcmd_ = cmd;
    char* pipechar = createplotcmd_.find( '|' );
    if ( pipechar )
    {
	viewplotcmd_ = pipechar + 2;
	*(pipechar-1) = '\0';
    }
    else
	viewplotcmd_.setEmpty();

    updateSummary();
}


BufferString uiMadagascarBldPlotCmd::getSummary() const
{
    BufferString retstr = createplotcmd_;
    if ( !viewplotcmd_.isEmpty() )
    {
	retstr += " | ";
	retstr += viewplotcmd_;
    }

    return retstr;
}


void uiMadagascarBldPlotCmd::doDlg( CallBacker* )
{
    uiDialog dlg(this, uiDialog::Setup(uiStrings::phrCreate(tr("plot command")),
						     mNoDlgTitle, mNoHelpKey) );
    uiString lbltxt = tr("Command to generate plot (e.g. sfwiggle)");
    uiLabel* lbl1 = new uiLabel( &dlg, lbltxt );
    uiLineEdit* genplotfld = new uiLineEdit( &dlg, "gen cmd" );
    genplotfld->setCompleter( cmdlist_, true );
    genplotfld->setvalue_( createplotcmd_.buf() );
    genplotfld->attach( rightTo, lbl1 );

    lbltxt = tr("Command to show plot (e.g. xtpen)");
    uiLabel* lbl2 = new uiLabel( &dlg, lbltxt );
    lbl2->attach( alignedBelow, lbl1 );
    BufferStringSet penlist;
    penlist.add( "xtpen" );
    uiLineEdit* viewplotfld = new uiLineEdit( &dlg, "view cmd" );
    viewplotfld->setCompleter( penlist, true );
    viewplotfld->setvalue_( viewplotcmd_ );
    viewplotfld->attach( rightTo, lbl2 );

    if ( !dlg.go() )
     return;

    createplotcmd_ = genplotfld->getvalue_();
    viewplotcmd_ = viewplotfld->getvalue_();
}



uiMadagascarBldCmd::uiMadagascarBldCmd( uiParent* p )
	: uiGroup(p,"Madagascar command builder")
	, cmdAvailable(this)
	, cmdisadd_(true)
	, groupfld_(0)
	, progfld_(0)
	, synopsfld_(0)
{
    bool allok = ODMad::PI().errMsg().isEmpty();
    if ( allok && !ODMad::PI().scanned() )
    {
	Executor* ex = ODMad::PI().getScanner();
	uiTaskRunner dlg( this );
	allok = TaskRunner::execute(&dlg,*ex) && ODMad::PI().defs().size() > 0;
	delete ex;
    }

    uiGroup* proggrp = new uiGroup( this, "Prog group" );
    if ( allok )
	createMainPart( proggrp );
    else
	new uiLabel( proggrp, ODMad::PI().errMsg() );

    uiGroup* lowgrp = createLowGroup();
    lowgrp->attach( ensureBelow, proggrp );

    postFinalize().notify( mCB(this,uiMadagascarBldCmd,onPopup) );
}


void uiMadagascarBldCmd::createMainPart( uiGroup* proggrp )
{
    uiGroup* selgrp = new uiGroup( 0, "Prog Selection group" );

    BufferStringSet grps; grps.add( sKeyAll ); grps.add( sKeySrchRes );
    BufferStringSet madgrps( ODMad::PI().groups() );
    madgrps.sort(); grps.add( madgrps, false );
    uiLabeledComboBox* groupfld = new uiLabeledComboBox( selgrp, grps,
                                                         tr("Group") );
    groupfld_ = groupfld->box();
    groupfld_->setToolTip( tr("Madagascar program group") );
    groupfld_->selectionChanged.notify( mCB(this,uiMadagascarBldCmd,groupChg) );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Program") );
    progfld_ = new uiListBox( selgrp, su );
    progfld_->attach( alignedBelow, groupfld );
    progfld_->selectionChanged.notify( mCB(this,uiMadagascarBldCmd,progChg) );
    progfld_->doubleClicked.notify( mCB(this,uiMadagascarBldCmd,dClick) );
    progfld_->setPrefHeightInChar( 12 );
    selgrp->setHAlignObj( progfld_ );

    uiGroup* infogrp = new uiGroup( 0, "Prog info group" );

    srchfld_ = new uiLineEdit( infogrp, "Search field" );
    srchfld_->setToolTip( tr("Search expression") );
    srchfld_->setPrefWidthInChar( 15 );
    srchfld_->returnPressed.notify( mCB(this,uiMadagascarBldCmd,doSearch) );
    uiToolButton* srchbut = new uiToolButton( infogrp, "search", tr("Search"),
				  mCB(this,uiMadagascarBldCmd,doSearch) );
    srchbut->attach( rightOf, srchfld_ );

    descfld_ = new uiLineEdit( infogrp, "Desc fld" );
    descfld_->attach( alignedBelow, srchfld_ );
    descfld_->setReadOnly( true );
    descfld_->setStretch( 2, 0 );
    commentfld_ = new uiTextEdit( infogrp, "Comments", true );
    commentfld_->attach( alignedBelow, descfld_ );
    commentfld_->setStretch( 1, 1 );
    commentfld_->setPrefHeightInChar( 10 );
    commentfld_->setPrefWidthInChar( 40 );

    infogrp->setHAlignObj( commentfld_ );

    uiSplitter* vspl = new uiSplitter( proggrp, "vert spl", true );
    vspl->addGroup( selgrp ); vspl->addGroup( infogrp );
}


uiGroup* uiMadagascarBldCmd::createLowGroup()
{
    uiGroup* lowgrp = new uiGroup( this, "Low grp" );

    if ( progfld_ )
    {
	synopsfld_ = new uiLineEdit( lowgrp, "Synopsis edit line" );
	synopsfld_->setReadOnly( true );
	synopsfld_->setStretch( 2, 0 );
	synopsfld_->setHSzPol( uiObject::WideVar );
	new uiLabel( lowgrp, tr("Synopsis"), synopsfld_ );
    }

    cmdfld_ = new uiLineEdit( lowgrp, "Command edit line" );
    cmdfld_->setStretch( 2, 0 );
    cmdfld_->setHSzPol( uiObject::MedVar );
    new uiLabel( lowgrp, tr("Command line"), cmdfld_ );
    if ( synopsfld_ )
	cmdfld_->attach( alignedBelow, synopsfld_ );

    useauxfld_ = new uiCheckBox( lowgrp, tr("Add Plot Command"),
				 mCB(this,uiMadagascarBldCmd,auxSel) );
    useauxfld_->attach( alignedBelow, cmdfld_ );

    auxcmdfld_ = new uiMadagascarBldPlotCmd( lowgrp );
    auxcmdfld_->attach( alignedBelow, useauxfld_ );
    auxcmdfld_->setStretch( 2, 0 );

    uiPushButton* addbut = new uiPushButton( lowgrp, tr("Add to flow"), true );
    addbut->setToolTip( tr("Add 'Command line' to process flow") );
    addbut->activated.notify( mCB(this,uiMadagascarBldCmd,doAdd) );
    addbut->attach( alignedBelow, auxcmdfld_ );
    addbut->setPrefWidthInChar( 16 );
    uiPushButton* edbut = new uiPushButton( lowgrp, tr("Replace in flow"),
                                            true );
    edbut->setToolTip( tr("Replace current command in"
                          " flow with 'Command line'") );
    edbut->activated.notify( mCB(this,uiMadagascarBldCmd,doEdit) );
    edbut->attach( rightOf, addbut );
    edbut->setPrefWidthInChar( 16 );

    lowgrp->setHAlignObj( synopsfld_ );
    auxSel(0);
    return lowgrp;
}


uiMadagascarBldCmd::~uiMadagascarBldCmd()
{
}


ODMad::Proc* uiMadagascarBldCmd::proc() const
{
    const char* text = cmdfld_->text();
    if ( !useauxfld_->isChecked() )
	return new ODMad::Proc( text );

    BufferString auxtxt = auxcmdfld_->getPlotCommand();
    return new ODMad::Proc( text, auxtxt.buf() );
}


void uiMadagascarBldCmd::setProc( const ODMad::Proc* inpproc )
{
    if ( !inpproc )
    {
	cmdfld_->setText( 0 );
	useauxfld_->setChecked( false );
	auxcmdfld_->setPlotCmd( 0 );
	return;
    }

    setProgName( inpproc->progName() );
    cmdfld_->setText( inpproc->getCommand() );
    const char* auxcmd = inpproc->auxCommand();
    useauxfld_->setChecked( auxcmd && *auxcmd );
    auxcmdfld_->setPlotCmd( auxcmd );
}


void uiMadagascarBldCmd::setProgName( const char* pnm )
{
    if ( !groupfld_ ) return;

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
    progfld_->setEmpty();
    const ObjectSet<ODMad::ProgDef>& defs = ODMad::PI().defs();
    for ( int idx=0; idx<defs.size(); idx++ )
    {
	const ODMad::ProgDef& def = *defs[idx];
	if ( !curgrp || def.group_ == curgrp )
	    progfld_->addItem( def.name_ );
    }
}


void uiMadagascarBldCmd::onPopup( CallBacker* )
{
    if ( !groupfld_ ) return;
    groupfld_->setText( sKeySrchRes );
    groupChg( 0 );
}


void uiMadagascarBldCmd::doSearch( CallBacker* )
{
    const BufferString srchkey( srchfld_->text() );
    if ( srchkey.isEmpty() ) return;

    ObjectSet<const ODMad::ProgDef> defs;
    ODMad::PI().search( srchkey, defs );
    progfld_->setEmpty();
    if ( defs.size() < 1 ) return;

    for ( int idx=0; idx<defs.size(); idx++ )
	progfld_->addItem( defs[idx]->name_ );

    progfld_->setCurrentItem( 0 );
    progChg( 0 );
}


void uiMadagascarBldCmd::auxSel( CallBacker* )
{
    auxcmdfld_->setSensitive( useauxfld_->isChecked() );
}


void uiMadagascarBldCmd::groupChg( CallBacker* c )
{
    const BufferString prognm = progfld_->getText();
    const BufferString* curgrp =
	find( ODMad::PI().groups().getStringSet(), groupfld_->text());
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
