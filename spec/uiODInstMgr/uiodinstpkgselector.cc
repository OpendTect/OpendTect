/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Aneesh
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uiodinstpkgselector.cc 7574 2013-03-26 13:56:34Z kristofer.tingdahl@dgbes.com $";

#include "uiodinstpkgselector.h"
#include "uiodinstmgrmod.h"
#include "odinstpkgprops.h"
#include "odinstpkgselmgr.h"
#include "odinsthtmlcomposer.h"
#include "uiodinstpkgmgr.h"
#include "uidesktopservices.h"
#include "file.h"
#include "filepath.h"

uiODInstPackageSelector::uiODInstPackageSelector( uiParent* p,
		ODInst::PkgSelMgr& pkgsel, const ODInst::PkgGroupSet& pgs )
    : uiTable(p,uiTable::Setup(1,6),"Package Selection")
    , pkgselmgr_(pkgsel)
    ,htmlcomp_(*new ODInst::HtmlComposer)
    ,notifier(this)
    
    
{
    fillTable();
    setAction();
}


uiODInstPackageSelector::~uiODInstPackageSelector()
{

}


void uiODInstPackageSelector::createTable()
{
    setNrRows( pkgselmgr_.size() );
    showGrid( false );
    for ( int i=0; i<nrCols(); i++ )
    {
	setColumnLabel( i, "" );
    }
    setColumnLabel( 1, "Name" );
    setColumnLabel( 2, "Installed" );
    setColumnLabel( 3, "Available" );
    setColumnLabel( 4, "Creator" );
    setColumnLabel( 5, "Action" );
    setColumnResizeMode( ResizeToContents );
    setColumnWidthInChar( 0, 3 );
    setColumnWidthInChar( 1, 70 );
    setColumnWidthInChar( 2, 13 );
    setColumnWidthInChar( 3, 13 );
    setColumnWidthInChar( 4, 15 );
    setColumnWidthInChar( 5, 15 );
    int r=0;
    for ( int idx=0; idx<pkgselmgr_.size(); idx++ )
    {
	const ODInst::PkgProps& pp  = (pkgselmgr_)[idx]->pp_;
	if ( pp.isHidden() )
	    continue;
	BufferString name = pp.usrname_;
	if ( name.size()<45 )
	{
	    setRowHeightInChar( r, 4 );
	}
	else
{
	    setRowHeightInChar( r, 5 );
	}	
	r++;
    }
}


void uiODInstPackageSelector::fillTable()
{
    createTable();
    int r = 0;
    pkgsel_.erase();
    
    for ( int idx=0; idx<pkgselmgr_.size(); idx++ )
    {
	const ODInst::PkgProps& pp  = (pkgselmgr_)[idx]->pp_;
	if ( pp.isHidden() )
	    continue;
	pkgsel_ +=  (pkgselmgr_)[idx];
	checkboxlist_ = new uiCheckBox( (uiParent*)this, " " );
	setCellObject( RowCol(r,0), checkboxlist_ );
	if ( pkgselmgr_.isSelected(pp) )
	    checkboxlist_->setChecked( true );
	checkboxlist_->activated.notify( 
		mCB(this,uiODInstPackageSelector,updateActionCB) );
	edittxt_ = new uiTextBrowser( (uiParent*)this, "html", 10, false );
	edittxt_->setHtmlText( htmlcomp_.genHTMLText(pp) );
	edittxt_->setLinkBehavior(uiTextBrowser::None);
	edittxt_->linkClicked.notify(mCB(this, uiODInstPackageSelector ,linkClickCB) );
	edittxt_->setMinimumWidth(400);
	edittxt_->hideFrame();
	edittxt_->hideScrollBarHorizontal();
	setCellObject( RowCol(r,1), edittxt_ );
	setText( RowCol(r,2), pkgselmgr_.version(pp,true).fullName() );
	setText( RowCol(r,3), (pkgselmgr_)[idx]->pp_.ver_.fullName() );

	
	creatortxt_ = new uiTextBrowser( 0, "html", 10, false );
	creatortxt_->setHtmlText( htmlcomp_.generateHTMLCreator(pp) );
	setCellObject( RowCol(r,4), creatortxt_ );
	creatortxt_->setMaximumWidth(100);
	creatortxt_->hideFrame();
	creatortxt_->hideScrollBarVertical();
	creatortxt_->setLinkBehavior(uiTextBrowser::None);
	creatortxt_->linkClicked.notify(mCB(this, uiODInstPackageSelector ,creatorClickCB) );
	r++;
    }
    setNrRows( r );
}


void uiODInstPackageSelector::setAction()
{
    #define mUpdateAction( rr,c,txt,rd,g,b ) \
	setText( RowCol(rr,c), txt ); \
	setColor( RowCol(rr,c), Color(rd,g,b) ); \
    
    for ( int ridx=0; ridx<pkgsel_.size(); ridx++ )
    {
	const ODInst::PkgProps& pp = (pkgsel_)[ridx]->pp_;
	const ODInst::PkgSelMgr::ReqAction reqact = (pkgselmgr_).reqAction(pp);
	
	if ( reqact == 0 )
	{
	    mUpdateAction( ridx, 5, " No Action ", 50, 150, 250 )
	}
	else if ( reqact == 1 )
	{
	    mUpdateAction( ridx, 5, " Install ", 50, 250, 100 )
	}
	else if ( reqact == 2 )
	{   
	    mUpdateAction( ridx, 5, " Update ", 50, 250, 100 )
	}
	else if ( reqact == 3 )
	{
	    mUpdateAction( ridx, 5, " UnInstall ", 250,75,75 )
	}
	else if ( reqact == 4 )
	{
	    setText( RowCol( ridx, 5 ), " Force Install " );   
	}
    }	
}


void uiODInstPackageSelector::updateActionCB( CallBacker* cb )
{
    mDynamicCastGet( uiCheckBox*,checkbox,cb );
    if ( !checkbox )
	return;
    RowCol rc = getCell( checkbox );
    const int ridx = rc.row;
    const ODInst::PkgProps& pp  = (pkgsel_)[ridx]->pp_;
    pkgselmgr_.setSelected( pp, checkbox->isChecked() ); 
    NotifyStopper nsi( checkbox->activated ); 
    updateSelection();
    setAction();
}


void uiODInstPackageSelector::updateSelection()
{
    for ( int idx=0; idx<nrRows(); idx++ )
    {
	const ODInst::PkgProps& pp  = (pkgsel_)[idx]->pp_;
	mDynamicCastGet(uiCheckBox*,checkbox,getCellObject(RowCol(idx,0)));
	if ( checkbox )
	    checkbox->setChecked( pkgselmgr_.isSelected( pp ) );
    }
}


void uiODInstPackageSelector::linkClickCB( CallBacker* cb )
{
    mDynamicCastGet( uiTextBrowser*,textbrowse,cb );
    if ( !textbrowse )
	return;
    RowCol rc = getCell( textbrowse );
    const int ridx = rc.row;
    notifier.trigger(pkgsel_[ridx]);   
}


void uiODInstPackageSelector::creatorClickCB( CallBacker* cb )
{ 
    mDynamicCastGet( uiTextBrowser*,textbrowse,cb );
    if ( !textbrowse )
	return;
    RowCol rc = getCell( textbrowse );
    const int ridx = rc.row;
    BufferString creatorlink = "http://www.";
    creatorlink.add(pkgsel_[ridx]->pp_.creator_->url_);
    uiDesktopServices::openUrl( creatorlink );   
}




