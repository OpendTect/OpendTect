/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          5-11-2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipsviewersettingdlg.h"

#include "uibutton.h"
#include "uipsviewerscalingtab.h"
#include "uipsviewerappearancetab.h"
#include "uipsviewershapetab.h"
#include "uipsviewerpreproctab.h"
#include "visflatviewer.h"
#include "visprestackdisplay.h"
#include "prestackprocessor.h"

namespace PreStackView
{


uiViewer3DSettingDlg::uiViewer3DSettingDlg( uiParent* p, 
	visSurvey::PreStackDisplay& viewer, uiViewer3DMgr& mgr, 
	PreStack::ProcessManager& prepromgr )
    : uiTabStackDlg( p, uiDialog::Setup( viewer.getObjectName(), 
		"Prestack display properties", "50.0.8").modal(false) ) 
    , preproctab_( 0 )		     
{
    shapetab_ = new uiViewer3DShapeTab( tabParent(), viewer, mgr );
    addGroup( shapetab_ );

    apptab_ = new uiViewer3DAppearanceTab( tabParent(), viewer, mgr );
    addGroup( apptab_ );

    scaletab_ = new uiViewer3DScalingTab( tabParent(), viewer, mgr );
    addGroup( scaletab_ );

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
       apptab_->saveAsDefault( true );
       scaletab_->saveAsDefault( true );
       shapetab_->saveAsDefault( true );
   }

   if ( applytoallfld_->isChecked() )
   {
       apptab_->applyToAll( true );
       scaletab_->applyToAll( true );
       if ( preproctab_ ) 
	   preproctab_->applyToAll( true );
       shapetab_->applyToAll( true );
   }

   return uiTabStackDlg::acceptOK(cb);
}


}; //namespace

