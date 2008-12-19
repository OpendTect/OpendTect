/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          5-11-2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewersettingdlg.cc,v 1.9 2008-12-19 21:58:00 cvsyuancheng Exp $";

#include "uipsviewersettingdlg.h"

#include "uibutton.h"
#include "uipsviewercoltab.h"
#include "uipsviewershapetab.h"
#include "uipsviewerpreproctab.h"
#include "visprestackviewer.h"
#include "visflatviewer.h"
#include "prestackprocessor.h"

namespace PreStackView
{


uiViewerSettingDlg::uiViewerSettingDlg( uiParent* p, 
	PreStackView::Viewer& viewer, uiViewerMgr& mgr, 
	PreStack::ProcessManager& prepromgr )
    : uiTabStackDlg( p, uiDialog::Setup( viewer.getObjectName(), 
		"Prestack display properties", "50.0.8") ) 
    , preproctab_( 0 )		     
{
    shapetab_ = new uiViewerShapeTab( tabParent(), viewer, mgr );
    addGroup( shapetab_ );

    coltab_ = new uiViewerColTab( tabParent(), viewer, mgr );
    addGroup( coltab_ );

    if ( viewer.is3DSeis() )
    {
    	preproctab_ = new uiViewerPreProcTab(tabParent(),viewer,mgr,prepromgr);	
    	addGroup( preproctab_ );
    }

    applytoallfld_ = new uiCheckBox(this,"&Apply to all viewers");
    applytoallfld_->attach( centeredBelow, tabObject() );
    
    enableSaveButton( "Save as Default" );
}


bool uiViewerSettingDlg::acceptOK( CallBacker* cb )
{
   if ( saveButtonChecked() )
   {
       coltab_->saveAsDefault( true );
       shapetab_->saveAsDefault( true );
   }

   if ( applytoallfld_->isChecked() )
   {
       coltab_->applyToAll( true );
       if ( preproctab_ ) 
	   preproctab_->applyToAll( true );
       shapetab_->applyToAll( true );
   }

   return uiTabStackDlg::acceptOK(cb);
}


}; //namespace

