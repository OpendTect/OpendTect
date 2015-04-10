/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		April 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";


#include "uibasemapsettings.h"

#include "settings.h"
#include "uibasemapwin.h"
#include "uicolor.h"
#include "uigraphicsview.h"
#include "uisellinest.h"
#include "uisurvmap.h"

uiBasemapSettingsDlg::uiBasemapSettingsDlg( uiParent* p, uiBasemapView& mgr )
    : uiDialog(p,uiDialog::Setup(tr("Basemap Settings"),mNoDlgTitle,
				      mNoHelpKey))
    , setts_(Settings::fetch("basemap"))
    , basemapvw_(mgr)
    , originalbgcol_(mgr.view().uibackgroundColor())
    , originallst_(basemapvw_.getSurveyBox()->getLineStyle())
{
    lsfld_ = new uiSelLineStyle( this, originallst_, "Survey Outline Style" );
    lsfld_->changed.notify(
		mCB(this,uiBasemapSettingsDlg,changeSurvBoudColCB) );

    uiColorInput::Setup stu( originalbgcol_ ); stu.lbltxt( "Background Color" );
    colbgfld_ = new uiColorInput( this, stu );
    colbgfld_->colorChanged.notify(
		mCB(this,uiBasemapSettingsDlg,changeBgColorCB) );
    colbgfld_->attach( alignedBelow, lsfld_->attachObj() );
}


uiBasemapSettingsDlg::~uiBasemapSettingsDlg()
{
}


bool uiBasemapSettingsDlg::acceptOK( CallBacker* )
{
    // fill IOPar
    setts_.write();
    return true;
}


bool uiBasemapSettingsDlg::rejectOK( CallBacker* )
{
    basemapvw_.getSurveyBox()->setLineStyle( originallst_ );
    basemapvw_.view().setBackgroundColor( originalbgcol_ );

    return true;
}


void uiBasemapSettingsDlg::changeSurvBoudColCB( CallBacker* )
{
    basemapvw_.getSurveyBox()->setLineStyle( lsfld_->getStyle() );
    basemapvw_.getSurveyBox()->updateStyle();
}


void uiBasemapSettingsDlg::changeBgColorCB( CallBacker* )
{
    basemapvw_.view().setBackgroundColor( colbgfld_->color() );
}
