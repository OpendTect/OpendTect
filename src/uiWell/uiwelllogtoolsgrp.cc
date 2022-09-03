/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelllogtoolsgrp.h"

#include "uilabel.h"
#include "uiscrollarea.h"
#include "uiwelllogdisplay.h"
#include "uiwelllogtools.h"
#include "welllog.h"
#include "welllogset.h"

static const int cPrefWidth = 150;
static const int cPrefHeight = 450;
static const int cPrefVSpacing = 2;
static const int cWellSep = 5;
static const int cDefNrLogs = 6;

uiWellLogToolWinGrp::uiWellLogToolWinGrp( uiParent* p,
			  const ObjectSet<WellLogToolData>& logdatas )
    : uiGroup(p, "Log Tools Display")
    , logdatas_(logdatas)
{}


uiWellLogToolWinGrp::~uiWellLogToolWinGrp()
{}


uiODWellLogToolWinGrp::uiODWellLogToolWinGrp( uiParent* p,
			  const ObjectSet<WellLogToolData>& logdatas )
    :uiWellLogToolWinGrp(p, logdatas)
{
    auto* sa = new uiScrollArea( this );
    sa->limitHeight(true);
    uiGroup* displaygrp = new uiGroup( nullptr, "Well display group" );
    displaygrp->setHSpacing( cWellSep );

    zdisplayrg_ = logdatas_[0]->getMDRange();
    uiGroup* prevgrp = new uiGroup( displaygrp, "Empty group" );
    int initialwidth = cWellSep;
    int totalwidth = cWellSep;
    int initialheight = cPrefHeight;
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	const WellLogToolData& logdata = *logdatas_[idx];
	auto* wellgrp  = new uiGroup( displaygrp, "Well group" );
	if ( prevgrp )
	    wellgrp->attach( rightOf, prevgrp );

	wellgrp->setHSpacing( 1 );
	wellgrp->setVSpacing( cPrefVSpacing );
	wellgrp->setStretch( 0, 2 );
	auto* wellnm = new uiLabel( wellgrp, toUiString(logdata.wellName()) );
	wellnm->setVSzPol( uiObject::Small );
	totalwidth += cWellSep;
	if ( logdisps_.size() <= cDefNrLogs )
	    initialwidth += cWellSep;

	if ( idx == 0 )
	    initialheight += wellnm->height() + 2*cPrefVSpacing;

	for ( int idlog=0; idlog<logdata.inpLogs().size(); idlog++ )
	{
	    uiWellLogDisplay::Setup su;
	    su.samexaxisrange_ = true;
	    auto* ld = new uiWellLogDisplay( wellgrp, su );
	    ld->setPrefWidth( cPrefWidth );
	    ld->setPrefHeight( cPrefHeight );
	    ld->setStretch( 0, 2 );
	    zdisplayrg_.include( logdata.getMDRange() );
	    if ( idlog )
		ld->attach( rightOf, logdisps_[logdisps_.size()-1] );

	    ld->attach( ensureBelow, wellnm );
	    logdisps_ += ld;
	    totalwidth += cPrefWidth + 1;
	    if ( logdisps_.size() <= cDefNrLogs )
		initialwidth += cPrefWidth + 1;
	}

	prevgrp = wellgrp;
    }

    zdisplayrg_.sort();
    displaygrp->attachObj()->setMinimumWidth( totalwidth );
    sa->setObject( displaygrp->attachObj() );
    sa->setMinimumHeight( initialheight );
    sa->setObjectResizable( true );

    sa->setPrefWidth( initialwidth );
}


uiODWellLogToolWinGrp::~uiODWellLogToolWinGrp()
{}


void uiODWellLogToolWinGrp::displayLogs()
{
    int nrdisp = 0;
    for ( int idx=0; idx<logdatas_.size(); idx++ )
    {
	const ObjectSet<const Well::Log>& inplogs =
				    logdatas_.get(idx)->inpLogs();
	ObjectSet<Well::Log> outplogs;
	BufferStringSet lognms;
	for ( const auto* log : logdatas_.get(idx)->inpLogs() )
	    lognms.add( log->name() );

	for ( const auto* lognm : lognms )
	{
	    const Well::Log* log = logdatas_[idx]->logs().getLog(
								lognm->buf() );
	    outplogs += cCast(Well::Log*,log);
	}

	for ( int idlog=0; idlog<inplogs.size(); idlog++ )
	{
	    uiWellLogDisplay* ld = logdisps_[nrdisp];
	    uiWellLogDisplay::LogData* wld = &ld->logData( true );
	    wld->setLog( inplogs[idlog] );
	    wld->disp_.setColor( OD::Color::stdDrawColor( 1 ) );
	    wld->zoverlayval_ = 1;

	    wld = &ld->logData( false );
	    wld->setLog( outplogs.validIdx(idlog) ? outplogs[idlog] : 0 );
	    wld->xrev_ = false;
	    wld->zoverlayval_ = 2;
	    wld->disp_.setColor( OD::Color::stdDrawColor( 0 ) );

	    ld->setZRange( zdisplayrg_ );
	    ld->reDraw();
	    nrdisp ++;
	}
    }
}
