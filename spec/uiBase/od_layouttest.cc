/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "uimain.h"

#include "moddepmgr.h"
#include "prog.h"


class uiLayoutDlg : public uiDialog
{
public:
			uiLayoutDlg(uiParent*);

protected:

    uiLineEdit*		smallfld_;
    uiLineEdit*		medfld_;
    uiLineEdit*		widefld_;

    uiLineEdit*		smallvarfld_;
    uiLineEdit*		medvarfld_;
    uiLineEdit*		widevarfld_;

    uiLineEdit*		smallmaxfld_;
    uiLineEdit*		medmaxfld_;
    uiLineEdit*		widemaxfld_;

    uiLineEdit*		smallflda_;
    uiLineEdit*		smallfldb_;
};


static uiLineEdit* createFld( uiParent* p, const char* txt )
{
    auto* le = new uiLineEdit( p, txt );
    if ( txt )
	new uiLabel( p, toUiString(txt), le );
    return le;
}


uiLayoutDlg::uiLayoutDlg( uiParent* p )
    : uiDialog(p,Setup(toUiString("Layout Test"),mNoDlgTitle,mTODOHelpKey))
{
    smallfld_ = createFld( this, "Small" );
    smallfld_->setHSzPol( uiObject::Small );

    smallflda_ = createFld( this, "Small A and B" );
    smallflda_->setHSzPol( uiObject::Small );
    smallflda_->attach( alignedBelow, smallfld_ );
    smallfldb_ = createFld( this, nullptr );
    smallfldb_->setHSzPol( uiObject::Small );
    smallfldb_->attach( rightTo, smallflda_ );

    medfld_ = createFld( this, "Medium" );
    medfld_->setHSzPol( uiObject::Medium );
    medfld_->attach( alignedBelow, smallflda_ );

    widefld_ = createFld( this, "Wide" );
    widefld_->setHSzPol( uiObject::Wide );
    widefld_->attach( alignedBelow, medfld_ );

    smallvarfld_ = createFld( this, "SmallVar" );
    smallvarfld_->setHSzPol( uiObject::SmallVar );
    smallvarfld_->attach( alignedBelow, widefld_ );

    medvarfld_ = createFld( this, "MedVar" );
    medvarfld_->setHSzPol( uiObject::MedVar );
    medvarfld_->attach( alignedBelow, smallvarfld_ );

    widevarfld_ = createFld( this, "WideVar" );
    widevarfld_->setHSzPol( uiObject::WideVar );
    widevarfld_->attach( alignedBelow, medvarfld_ );

    smallmaxfld_ = createFld( this, "SmallMax" );
    smallmaxfld_->setHSzPol( uiObject::SmallMax );
    smallmaxfld_->attach( alignedBelow, widevarfld_ );

    medmaxfld_ = createFld( this, "MedMax" );
    medmaxfld_->setHSzPol( uiObject::MedMax );
    medmaxfld_->attach( alignedBelow, smallmaxfld_ );

    widemaxfld_ = createFld( this, "WideMax" );
    widemaxfld_->setHSzPol( uiObject::WideMax );
    widemaxfld_->attach( alignedBelow, medmaxfld_ );
}



int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "General" );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiBase" );
    PtrMan<uiDialog> mw = new uiLayoutDlg( nullptr );
    app.setTopLevel( mw );
    PIM().loadAuto( true );
    mw->go();

    return app.exec();
}
