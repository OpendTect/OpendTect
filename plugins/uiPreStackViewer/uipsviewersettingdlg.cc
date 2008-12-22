/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          5-11-2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewersettingdlg.cc,v 1.11 2008-12-22 19:25:37 cvsyuancheng Exp $";

#include "uipsviewersettingdlg.h"

#include "uibutton.h"
#include "uipsviewerappeartab.h"
#include "uipsviewercoltab.h"
#include "uipsviewershapetab.h"
#include "uipsviewerpreproctab.h"
#include "visprestackviewer.h"
#include "visflatviewer.h"
#include "prestackprocessor.h"

namespace PreStackView
{


uiViewer3DSettingDlg::uiViewer3DSettingDlg( uiParent* p, 
	PreStackView::Viewer3D& viewer, uiViewer3DMgr& mgr, 
	PreStack::ProcessManager& prepromgr )
    : uiTabStackDlg( p, uiDialog::Setup( viewer.getObjectName(), 
		"Prestack display properties", "50.0.8") ) 
    , preproctab_( 0 )		     
{
    shapetab_ = new uiViewer3DShapeTab( tabParent(), viewer, mgr );
    addGroup( shapetab_ );

    apptab_ = new uiViewer3DAppearTab( tabParent(), viewer, mgr );
    addGroup( apptab_ );

    coltab_ = new uiViewer3DColTab( tabParent(), viewer, mgr );
    addGroup( coltab_ );

    if ( viewer.is3DSeis() )
    {
    	preproctab_ = 
	    new uiViewer3DPreProcTab( tabParent(), viewer, mgr, prepromgr );
	addGroup( preproctab_ );
    }

    applytoallfld_ = new uiCheckBox(this,"&Apply to all viewers");
    applytoallfld_->attach( centeredBelow, tabObject() );
    
    enableSaveButton( "Save as Default" );
}


bool uiViewer3DSettingDlg::acceptOK( CallBacker* cb )
{
   if ( saveButtonChecked() )
   {
       coltab_->saveAsDefault( true );
       apptab_->saveAsDefault( true );
       shapetab_->saveAsDefault( true );
   }

   if ( applytoallfld_->isChecked() )
   {
       coltab_->applyToAll( true );
       apptab_->applyToAll( true );
       if ( preproctab_ ) 
	   preproctab_->applyToAll( true );
       shapetab_->applyToAll( true );
   }

   return uiTabStackDlg::acceptOK(cb);
}


}; //namespace

