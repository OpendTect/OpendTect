/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipicksetman.h"
#include "uipicksetmgr.h"

#include "uiioobjselgrp.h"
#include "uigisexp.h"
#include "uigisexpdlgs.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "ioman.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "picksettr.h"
#include "pickset.h"
#include "separstr.h"
#include "survinfo.h"

static const int cPointSetMenu		= 0;
static const int cPolygonMenu		= 1;

mDefineInstanceCreatedNotifierAccess(uiPickSetMan)

static IOObjContext getIOObjContext( const char* fixedtrkey )
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    if ( fixedtrkey )
	ctxt.fixTranslator( fixedtrkey );

    const FileMultiString fms( sKey::PickSet(), sKey::Polygon() );
    ctxt.requireType( fms.str() );
    return ctxt;
}


uiPickSetMan::uiPickSetMan( uiParent* p, const char* fixedtrkey )
    : uiObjFileMan(p,Setup(uiStrings::phrManage( toUiString("%1/%2")
				.arg(uiStrings::sPointSet(mPlural))
				.arg(uiStrings::sPolygon(mPlural))),
			   mODHelpKey(mPickSetManHelpID))
			.nrstatusflds(1).modal(false),
	    getIOObjContext(fixedtrkey),sKey::Type())
{
    createDefaultUI();
    mergebut_ = addManipButton( "mergepicksets",
				uiStrings::phrMerge(uiStrings::sPointSet()),
				mCB(this,uiPickSetMan,mergeSets) );
    CallBack cb = mCB(this,uiPickSetMan,exportToGISCB);
    uiToolButton* gisbut = addExtraButton( uiGISExpStdFld::strIcon(),
					uiGISExpStdFld::sToolTipTxt(),
					cb );
    auto* menu = new uiMenu( tr("Export to GIS Format") );
    auto* psaction = new uiAction( uiStrings::sPointSet(), cb );
    menu->insertAction( psaction, cPointSetMenu );
    auto* pgaction = new uiAction( uiStrings::sPolygon(), cb );
    menu->insertAction( pgaction, cPolygonMenu );
    gisbut->setMenu( menu, uiToolButton::InstantPopup );

    mTriggerInstanceCreatedNotifier();
}


uiPickSetMan::~uiPickSetMan()
{}


void uiPickSetMan::addToMgr( const MultiID& id, uiString& msg )
{
    PtrMan<IOObj> obj = IOM().get( id );
    if ( !obj )
	return;

    RefMan<Pick::Set> ps = new Pick::Set;
    PickSetTranslator::retrieve( *ps, obj.ptr(), true, msg );
    Pick::Mgr().set( id, ps.ptr() );
}


void uiPickSetMan::exportToGISCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm || !uiGISExpStdFld::canDoExport(this) )
	return;

    const int itmid = itm->getID();
    uiGISExportDlg::Type exptype;
    if ( itmid==cPointSetMenu )
	exptype = uiGISExportDlg::Type::PointSet;
    else if ( itmid==cPolygonMenu )
	exptype = uiGISExportDlg::Type::Polygon;
    else
	return;

    TypeSet<MultiID> selids;
    getChosen( selids );
    RefObjectSet<const Pick::Set> gisdata;
    uiString errmsg;
    for ( const auto& id : selids )
    {
	const int idx = Pick::Mgr().indexOf( id );
	if ( idx < 0 )
	{
	    addToMgr( id, errmsg );
	    const int idp = Pick::Mgr().indexOf( id );
	    if ( idp < 0 )
		continue;
	}

	ConstRefMan<Pick::Set> ps = Pick::Mgr().get( id );
	if ( !ps )
	    continue;

	const bool ispolygon = ps->isPolygon();
	if ( (itmid==cPointSetMenu && !ispolygon)
		|| (itmid==cPolygonMenu && ispolygon) )
	    gisdata.add( ps.ptr() );
    }

    if ( !errmsg.isEmpty() )
    {
	if ( gisdata.isEmpty() )
	    uiMSG().error( tr("Can't export some files."), errmsg);
	else
	    uiMSG().warning( errmsg );
    }

    uiGISExportDlg dlg( this, exptype, gisdata );
    dlg.go();
}


void uiPickSetMan::ownSelChg()
{
    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    uiString tt;
    if ( chsnnms.size() > 1 )
	tt = uiStrings::phrMerge( toUiString(chsnnms.getDispString(2)) );
    else
	tt = uiStrings::phrMerge(uiStrings::sPointSet());

    mergebut_->setToolTip( tt );
}


void uiPickSetMan::mkFileInfo()
{
    if ( !curioobj_ )
    {
	setInfo( "" );
	return;
    }

    BufferString typ = curioobj_->pars().find( sKey::Type() );
    const bool ispoly = typ.isEqual( sKey::Polygon() );
    RefMan<Pick::Set> ps = new Pick::Set( curioobj_->name(), ispoly );
    BufferString txt;
    uiString errmsg;
    if ( PickSetTranslator::retrieve(*ps,curioobj_,true,errmsg) )
    {
	if ( !errmsg.isEmpty() )
	    ErrMsg( errmsg );

	if ( typ.isEmpty() || typ == sKey::PickSet() )
	    typ = sKey::PointSet();

	txt.add( "Type: " ).add( typ );

	const int sz = ps->size();
	if ( sz < 1 )
	    txt.add( "\nSize: Empty" );
	else
	{
	    txt.add( "\nSize: " ).add( sz );
	    txt.add( ispoly ? ( sz > 1 ? " vertices" : " vertex" )
			    : ( sz > 1 ? " points" : " point" ) );
	    if ( !ispoly && ps->get(0).hasDir() )
		txt += " (with directions)";

	    if ( ispoly && sz > 2 )
	    {
		const float area = ps->getXYArea();
		if ( !mIsUdf(area) )
		{
		    txt.add( ", area: " )
		       .add( getAreaString(area,SI().xyInFeet(),
					   SI().nrXYDecimals(),true) );
		}
	    }
	}

	OD::Color col( ps->disp3d().color() );
	col.setTransparency( 0 );
	txt.add( "\nColor: " ).add( col.largeUserInfoString() );
	txt.add( "\nMarker size (pixels): " ).add( ps->disp3d().size() );
	txt.add( "\nMarker type: " ).add( MarkerStyle3D::getTypeString(
			sCast(MarkerStyle3D::Type,ps->disp3d().type())) );
	txt.add( "\n" );
    }

    txt.add( getFileInfo() );
    setInfo( txt );
}


void uiPickSetMan::mergeSets( CallBacker* )
{
    uiPickSetMgr mgr( this, Pick::Mgr() );
    MultiID curkey;
    if ( curioobj_ )
	curkey = curioobj_->key();

    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    mgr.mergeSets( curkey, &chsnnms );
    if ( !curkey.isUdf() )
	selgrp_->fullUpdate( curkey );
}
