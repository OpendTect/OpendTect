/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodvolproctreeitem.h"

#include "attribdesc.h"
#include "attribsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uiodseis2dtreeitem.h"
#include "uivispartserv.h"
#include "vissurvobj.h"
#include "volprocattrib.h"
#include "volprocchain.h"
#include "uivolprocchain.h"
#include "volproctrans.h"


namespace VolProc
{


void uiDataTreeItem::initClass()
{ uiODDataTreeItem::factory().addCreator( create, 0 ); }


uiDataTreeItem::uiDataTreeItem( const char* parenttype, const MultiID* key )
    : uiODDataTreeItem( parenttype )
    , selmenuitem_( m3Dots(tr("Select Setup")), true )
    , reloadmenuitem_( uiStrings::sReload(), true )
    , editmenuitem_( uiStrings::sEdit(), true )
    , mid_(key ? *key : MultiID::udf())
{
    editmenuitem_.iconfnm = VolProc::uiChain::pixmapFileName();
    reloadmenuitem_.iconfnm = "refresh";
    selmenuitem_.iconfnm = "selectfromlist";
}


uiDataTreeItem::~uiDataTreeItem()
{}


bool uiDataTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item!=uitreeviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv->getColTabSequence(displayID(),attribNr()) )
	return false;

    ODMainWin()->applMgr().updateColorTable( displayID(), attribNr() );

    return true;
}


uiODDataTreeItem* uiDataTreeItem::create( const Attrib::SelSpec& as,
					  const char* parenttype )
{
    if ( as.id().asInt()!=Attrib::SelSpec::cOtherAttrib().asInt() )
	return 0;

    BufferString attribnm;
    const char* defstr = as.defString();
    Attrib::Desc::getAttribName( defstr, attribnm );
    if ( attribnm != VolProc::ExternalAttribCalculator::sAttribName() )
	return 0;

    const char* parkey = VolProc::ExternalAttribCalculator::sKeySetup();
    BufferString setupmidstr;
    MultiID setupmid = MultiID::udf();
    if ( Attrib::Desc::getParamString(defstr,parkey,setupmidstr) )
	setupmid = MultiID( setupmidstr.buf() );

    return new uiDataTreeItem( parenttype, &setupmid );
}


#define mCreateMenu( func ) \
    mDynamicCastGet(MenuHandler*,menu,cb); \

void uiDataTreeItem::createMenu( MenuHandler* menu ,bool istb )
{
    uiODDataTreeItem::createMenu( menu, istb );

    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    mAddMenuOrTBItem( istb, 0, menu, &selmenuitem_, true, false );
    mAddMenuOrTBItem( istb, menu, menu, &reloadmenuitem_, ioobj, false );
    mAddMenuOrTBItem( istb, 0, menu, &editmenuitem_, ioobj, false );
}


void uiDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);

    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();

    if ( mnuid==selmenuitem_.id )
    {
	menu->setIsHandled( true );
	if ( !selectSetup() ) return;
    }

    if ( mnuid==selmenuitem_.id || mnuid==reloadmenuitem_.id )
    {
	menu->setIsHandled( true );
	visserv->calculateAttrib( displayID(), attribNr(), false );
    }
    else if ( mnuid==editmenuitem_.id )
    {
	StringView parenttype = parentType();
	const bool is2d = parenttype == typeid(uiOD2DLineTreeItem).name();
	applMgr()->doVolProc( mid_, is2d );
    }
}


bool uiDataTreeItem::selectSetup()
{
    PtrMan<IOObj> ioobj = IOM().get( mid_ );

    StringView parenttype = parentType();
    const bool is2d = parenttype == typeid(uiOD2DLineTreeItem).name();
    IOObjContext ioctxt = is2d ? VolProcessing2DTranslatorGroup::ioContext()
				: VolProcessingTranslatorGroup::ioContext();
    const CtxtIOObj ctxt( ioctxt, ioobj );
    uiIOObjSelDlg dlg( ODMainWin(), ctxt );
    if ( !dlg.go() || dlg.nrChosen() < 1 )
	return false;

    RefMan<VolProc::Chain> chain = new VolProc::Chain;
    uiString str;
    if ( VolProcessingTranslator::retrieve(*chain,ioobj,str) )
    {
	if ( !chain->areSamplesIndependent() )
	{
	    if ( !uiMSG().askGoOn(tr("The output of this setup is not "
			"sample-independent, and the output may not be "
			"the same as when computing the entire volume")) )
		return false;
	}
    }

    mid_ = dlg.chosenID();

    const BufferString def =
	VolProc::ExternalAttribCalculator::createDefinition( mid_ );
    Attrib::SelSpec spec( "VolProc", Attrib::SelSpec::cOtherAttrib(),
			  false, 0 );
    spec.setDefString( def.buf() );
    applMgr()->visServer()->setSelSpec( displayID(), attribNr(), spec );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    return true;
}


uiString uiDataTreeItem::createDisplayName() const
{
    uiString dispname = uiStrings::sRightClick();
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( ioobj )
	dispname = ioobj->uiName();

    return dispname;
}


void uiDataTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject(displayID()))
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}

	if ( !so->hasColor() )
	    displayMiniCtab( so->getColTabSequence(attribNr()) );
    }

    uiODDataTreeItem::updateColumnText( col );
}

} // namespace VolProc
