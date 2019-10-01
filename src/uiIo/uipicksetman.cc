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
#include "uidatapointset.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ioobjctxt.h"
#include "datapointset.h"
#include "draw.h"
#include "picksetmanager.h"
#include "picksettr.h"
#include "keystrs.h"
#include "survinfo.h"
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
		.arg( uiStrings::sPointSet(mPlural) )
		.arg( uiStrings::sPolygon(mPlural))),
	     mNoDlgTitle,
	     mODHelpKey(mPickSetManHelpID) )
		 .nrstatusflds(1).modal(false),
			getIOObjContext(fixedtrkey))
{
    createDefaultUI();
    edbut_ = selgrp_->getManipGroup()->addButton( "edit",
				    uiStrings::phrEdit(uiStrings::sPointSet()),
				    mCB(this,uiPickSetMan,edSetCB) );
    mergebut_ = selgrp_->getManipGroup()->addButton( "mergepicksets",
			    uiStrings::phrMerge(uiStrings::sPointSet(mPlural)),
				    mCB(this,uiPickSetMan,mergeSetsCB) );
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::ownSelChg()
{
    BufferStringSet nms;
    selgrp_->getChosen( nms );
    const bool singlechosen = nms.size() == 1;
    const bool multichosen = nms.size() > 1;

    edbut_->setToolTip( singlechosen ? uiStrings::phrEdit(nms.get(0))
				 : uiStrings::phrEdit(uiStrings::sPointSet()));
    edbut_->setSensitive( singlechosen );
    mergebut_->setToolTip( multichosen ?
		  uiStrings::phrMerge( nms.getDispString(3) )
		: uiStrings::phrMerge( uiStrings::sPointSet(mPlural) ) );
    mergebut_->setSensitive( multichosen );
}


bool uiPickSetMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( ioobj.key(), uirv );
    if ( !ps )
	{ inf.add( uirv ); return false; }

    const bool ispoly = ps->isPolygon();
    const BufferString cat = ps->category();
    uiString typestr;

    if ( ispoly )
	typestr = uiStrings::sPolygon();
    else if ( !cat.isEmpty() )
	typestr = toUiString(cat);
    else
	typestr = uiStrings::sPointSet();


    MonitorLock ml( *ps );
    const int sz = ps->size();
    uiString szstr;
    if ( sz < 1 )
	szstr = uiStrings::sEmpty();
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
		szstr.postFixWord( toUiString(" %1: %2")
			.arg( uiStrings::sArea().toLower() )
			.arg( getAreaString(area,SI().xyInFeet(),2,true) ) );
	}
    }

    szstr.embedFinalState();
    typestr.postFixWord( szstr );

    addObjInfo( inf, uiStrings::sType(), typestr );

    const Pick::Set::Disp disp = ps->getDisp();

    addObjInfo( inf, uiStrings::sColor(),
				disp.mkstyle_.color_.userInfoString(true) );
    addObjInfo( inf, tr("Marker size (pixels)"), disp.mkstyle_.size_ );
    addObjInfo( inf, tr("Marker Type"), OD::MarkerStyle3D::TypeDef()
			.getUiStringForIndex(disp.mkstyle_.type_) );
    if ( SI().has2D() && SI().has3D() )
    {
	const bool has2d = ps->has2D();
	const bool has3d = ps->has3D();
	uiString pickedonstr = has3d ? tr("3D Seismics") : tr("2D Seismics");
	if ( has2d && has3d )
	    pickedonstr = tr("Both 2D and 3D Seismics");
	addObjInfo( inf, tr("Picked on"), pickedonstr );
    }

    return true;
}


void uiPickSetMan::edSetCB( CallBacker* )
{
    if ( !curioobj_ )
	return;

    //TODO make a proper table-based editor that monitors the Pick::Set
    const DBKey dbky( curioobj_->key() );
    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( dbky, uirv );
    if ( !ps )
	{ uiMSG().error( uirv ); return; }

    const bool is2donly = ps->hasOnly2D();
    RefMan<DataPointSet> dps = new DataPointSet( is2donly );
    Pick::SetIter it( *ps );
    while ( it.next() )
    {
	const auto& loc = it.get();
	DataPointSet::Pos pos;
	const Coord coord = loc.pos().getXY();
	if ( !is2donly )
	    pos.set( loc.binID(), coord );
	else
	    pos.set( loc.bin2D(), coord );
	pos.setZ( loc.z() );
	DataPointSet::DataRow dr( pos );
	dps->addRow( dr );
    }
    dps->dataChanged();

    const bool editable = (is2donly || ps->hasOnly3D())
			&& !ps->haveDirections() && !ps->haveTexts();
    uiDataPointSet::Setup su( uiStrings::phrEdit(dbky.name()), true );
    su.isconst( editable );
    su.canaddrow( true );
    su.directremove( true );
    su.allowretrieve( false );

    uiDataPointSet uidp( this, *dps, su );
    if ( !uidp.go() || !editable )
	return;

    uiMSG().error( mINTERNAL("TODO - implement save edited data") );
}


void uiPickSetMan::mergeSetsCB( CallBacker* )
{
    DBKey curid;
    if ( curioobj_ )
	curid = curioobj_->key();

    uiMergePickSets dlg( this, curid );
    if ( dlg.go() )
	selgrp_->fullUpdate( curid );
}
