/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uipicksetman.h"
#include "uipicksettools.h"

#include "uibutton.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ioobjctxt.h"
#include "draw.h"
#include "picksetmanager.h"
#include "picksettr.h"
#include "keystrs.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiPickSetMan)

static IOObjContext getIOObjContext( const char* fixedtrkey )
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    if ( fixedtrkey )
	ctxt.fixTranslator( fixedtrkey );

    BufferString types = sKey::PickSet();
    types.add( "`" ).add( sKey::Polygon() ).add( "`" );
    ctxt.toselect_.require_.set( sKey::Type(), types.buf() );
    return ctxt;
}

uiPickSetMan::uiPickSetMan( uiParent* p, const char* fixedtrkey )
    : uiObjFileMan(p,uiDialog::Setup(
             uiStrings::phrManage( toUiString("%1/%2")
                .arg( uiStrings::sPickSet(mPlural) )
                .arg( uiStrings::sPolygon(mPlural))),
             mNoDlgTitle,
             mODHelpKey(mPickSetManHelpID) )
                 .nrstatusflds(1).modal(false),
			getIOObjContext(fixedtrkey))
{
    createDefaultUI();
    mergebut_ = selgrp_->getManipGroup()->addButton( "mergepicksets",
				    uiStrings::phrMerge(uiStrings::sPickSet()),
				    mCB(this,uiPickSetMan,mergeSets) );
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::ownSelChg()
{
    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    uiString tt;
    if ( chsnnms.size() > 1 )
	tt = uiStrings::phrMerge( toUiString(chsnnms.getDispString(2)) );
    else
	tt = uiStrings::phrMerge(uiStrings::sPickSet());
    mergebut_->setToolTip( tt );
}


bool uiPickSetMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( ioobj.key(), uirv );
    if ( !ps )
	{ inf.add( uirv ); return false; }

    const bool ispoly = ps->isPolygon();
    const BufferString cat = ps->category();
    uiString typestr = uiStrings::sType();
    if ( ispoly )
	typestr.addMoreInfo( uiStrings::sPolygon() );
    else if ( !cat.isEmpty() )
	typestr.addMoreInfo( toUiString(cat) );
    else
	typestr.addMoreInfo( uiStrings::sPickSet() );

    MonitorLock ml( *ps );
    const int sz = ps->size();
    uiString szstr = toUiString("<%1>");
    if ( sz < 1 )
	szstr.set( toUiString("<%1>").arg( uiStrings::sEmpty() ) );
    else
    {
	szstr = toUiString("%1 %2")
		    .arg( sz )
		    .arg( ispoly ? uiStrings::sVertex(mPlural)
				 : uiStrings::sPick(mPlural) );
	if ( !ispoly && ps->haveDirections() )
	    szstr.postFixWord( tr("with directions").parenthesize() );
	if ( ispoly && sz > 2 )
	{
	    const float area = ps->getXYArea();
	    if ( !mIsUdf(area) )
		szstr.postFixWord( toUiString(" %1=%2")
			.arg( uiStrings::sArea().toLower() )
			.arg( area ) );
	}
	szstr.embed( "<", ">" );
    }
    inf.add( typestr.postFixWord(szstr) );

    const Pick::Set::Disp disp = ps->getDisp();
    inf.add( uiStrings::sColor()
	     .addMoreInfo( disp.mkstyle_.color_.userInfoString(true) ) );

    inf.add( tr("Marker size (pixels)")
	     .addMoreInfo( toUiString(disp.mkstyle_.size_)) );

    inf.add( tr("Marker type").addMoreInfo( OD::MarkerStyle3D::TypeDef()
			.getUiStringForIndex(disp.mkstyle_.type_) ) );

    return true;
}


void uiPickSetMan::mergeSets( CallBacker* )
{
    DBKey curid;
    if ( curioobj_ )
	curid = curioobj_->key();

    uiMergePickSets dlg( this, curid );
    if ( dlg.go() )
	selgrp_->fullUpdate( curid );
}
