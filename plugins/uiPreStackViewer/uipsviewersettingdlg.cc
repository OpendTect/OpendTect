/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          5-11-2007
 RCS:           $Id: uipsviewersettingdlg.cc,v 1.5 2008-08-28 16:12:56 cvsyuancheng Exp $
________________________________________________________________________

-*/

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


uiPSViewerSettingDlg::uiPSViewerSettingDlg( uiParent* p, 
	PreStackViewer& viewer, uiPSViewerMgr& mgr, 
	PreStack::ProcessManager& prepromgr )
    : uiTabStackDlg( p, uiDialog::Setup( viewer.getObjectName(), 
		"Prestack display properties", "50.0.8") ) 
    , preproctab_( 0 )		     
{
    shapetab_ = new uiPSViewerShapeTab( tabParent(), viewer, mgr );
    addGroup( shapetab_ );

    coltab_ = new uiPSViewerColTab( tabParent(), viewer, mgr );
    addGroup( coltab_ );

    if ( viewer.is3DSeis() )
    {
    	preproctab_ = new uiPSViewerPreProcTab( tabParent(), viewer, mgr,
	       prepromgr );	
    	addGroup( preproctab_ );
    }

    applytoallfld_ = new uiCheckBox(this,"&Apply to all viewers");
    applytoallfld_->attach( centeredBelow, tabObject() );
    
    enableSaveButton( "Save as Default" );
}


bool uiPSViewerSettingDlg::acceptOK( CallBacker* cb )
{
   if ( saveButtonChecked() )
       shapetab_->saveAsDefault( true );

   if ( applytoallfld_->isChecked() )
   {
       shapetab_->applyToAll( true );
       coltab_->applyToAll( true );
       if ( preproctab_ ) 
	   preproctab_->applyToAll( true );
   }

   return uiTabStackDlg::acceptOK(cb);
}


}; //namespace

