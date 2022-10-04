/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicolor.h"

#include "uibody.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uiparentbody.h"
#include "uipixmap.h"
#include "uispinbox.h"
#include "uistrings.h"

#include "q_uiimpl.h"

#include <QApplication>
#include <QColorDialog>
#include <QLabel>

mUseQtnamespace

#define mGlobalQColorDlgCmdRecId 1

static int beginCmdRecEvent( const char* windowtitle )
{
    uiMainWin* carrier = uiMain::instance().topLevel();
    if ( !carrier )
	return -1;

    BufferString msg( "QColorDlg " );
    msg += windowtitle;

    return carrier->beginCmdRecEvent( mGlobalQColorDlgCmdRecId, msg );
}

static void endCmdRecEvent( int refnr, bool ok, const OD::Color& col,
			    bool withtransp )
{
    BufferString msg( "QColorDlg " );
    if ( ok )
    {
	msg += (int) col.r(); msg += " ";
	msg += (int) col.g(); msg += " ";
	msg += (int) col.b(); msg += " ";
	if ( withtransp )
	    msg += (int) col.t();
    }

    uiMainWin* carrier = uiMain::instance().topLevel();
    if ( carrier )
	carrier->endCmdRecEvent( mGlobalQColorDlgCmdRecId, refnr, msg );
}


uiColorInput::~uiColorInput()
{}


uiString uiColorInput::sSelColor()
{ return uiStrings::phrSelect( uiStrings::sColor() ); }


bool selectColor( OD::Color& col, uiParent* parnt,
		  uiString txt, bool withtransp )
{
    return uiColorInput::selectColor( col, parnt, txt, withtransp );
}



bool uiColorInput::selectColor( OD::Color& col, uiParent* parnt,
				uiString txt, bool withtransp )
{
    QWidget* qparent = parnt ? parnt->pbody()->qwidget() : 0;
    if ( txt.isEmpty() ) txt = uiColorInput::sSelColor();

    BufferString addendum;
    const uiString wintitle = uiMainWin::uniqueWinTitle( txt, 0, &addendum );
    BufferString utfwintitle( txt.getFullString(), addendum );
    const int refnr = ::beginCmdRecEvent( utfwintitle );

    QColorDialog qdlg( QColor(col.r(),col.g(), col.b(),col.t()), qparent );
    qdlg.setWindowTitle( toQString(wintitle) );
    if ( withtransp )
    {
        qdlg.setOption( QColorDialog::ShowAlphaChannel );
        qdlg.setOption( QColorDialog::DontUseNativeDialog );
	QList<QLabel*> lbllst = qdlg.findChildren<QLabel*>("");
	bool found = false;
	foreach(QLabel* qlbl,lbllst)
	{
	    if ( qlbl->text() == "Alpha channel:" )
		{ qlbl->setText( "Transparency:" ); found = true; break; }
	}
	if ( !found )
	{
	  pFreeFnErrMsg("Implement support for label change in this Qt ver");
	}
    }

    const bool ok = qdlg.exec() == QDialog::Accepted;

    if ( ok )
    {
	QColor newcol = qdlg.selectedColor();
	col.set( newcol.red(), newcol.green(), newcol.blue(),
		 withtransp ? newcol.alpha() : col.t() );
    }

    ::endCmdRecEvent( refnr, ok, col, withtransp );
    return ok;
}


void setExternalColor( const OD::Color& col )
{
     QWidget* amw = qApp->activeModalWidget();
     QColorDialog* qcd = dynamic_cast<QColorDialog*>( amw );
     if ( qcd )
	 qcd->setCurrentColor( QColor(col.r(),col.g(),col.b(),
						col.t()) );
}


uiColorInput::uiColorInput( uiParent* p, const Setup& s, const char* nm )
	: uiGroup(p,"Color input")
	, color_(s.color_)
	, dlgtxt_(s.dlgtitle_)
	, dodrawbox_(0)
	, lbl_(0)
	, transpfld_(0)
	, descfld_(0)
	, selwithtransp_(s.transp_ == Setup::InSelector)
	, colorChanged(this)
	, doDrawChanged(this)
{
    if ( s.withcheck_ )
    {
	dodrawbox_ = new uiCheckBox( this, s.lbltxt_);
	dodrawbox_->setChecked( false );
	dodrawbox_->activated.notify( mCB(this,uiColorInput,dodrawSel) );
    }
    colbut_ = new uiPushButton( this,uiString::emptyString(), false );
    colbut_->setName( (nm && *nm)
	? nm
	: (!s.lbltxt_.isEmpty() ? s.lbltxt_.getFullString().buf() : "Color" ) );
    colbut_->activated.notify( mCB(this,uiColorInput,selCol) );
    if ( dodrawbox_ )
	colbut_->attach( rightOf, dodrawbox_ );

    if ( !dodrawbox_ && !s.lbltxt_.isEmpty() )
	lbl_ = new uiLabel( this, s.lbltxt_, colbut_ );

    uiLabeledSpinBox* lsb = 0;
    if ( s.transp_ == Setup::Separate )
    {
	lsb = new uiLabeledSpinBox( this, tr("Transp"), 0 );
	lsb->attach( rightOf, colbut_ );
	transpfld_ = lsb->box();
	transpfld_->setSuffix( tr("%") );
	transpfld_->setInterval( 0, 100, 1 );
	transpfld_->setHSzPol( uiObject::Small );
	transpfld_->valueChanged.notify( mCB(this,uiColorInput,transpChg) );
    }
    if ( s.withdesc_ )
    {
	descfld_ = new uiComboBox( this, OD::Color::descriptions(),
				    "Color description" );
	descfld_->setHSzPol( uiObject::Medium );
	TypeSet<OD::Color> colors = OD::Color::descriptionCenters();
	for ( int idx=0; idx<colors.size(); idx++ )
	{
	    uiPixmap pm( 15, 10 );
	    pm.fill( colors[idx] );
	    descfld_->setPixmap( idx, pm );
	}

	if ( lsb )
	    descfld_->attach( rightOf, lsb );
	else
	    descfld_->attach( rightOf, colbut_ );
	descfld_->selectionChanged.notify( mCB(this,uiColorInput,descSel) );
	descfld_->setHSzPol( uiObject::MedMax );
    }

    setColor( color_ );
    if ( lbl_ || dodrawbox_ )
	setHAlignObj( colbut_ );
    else if ( transpfld_ )
	setHAlignObj( transpfld_ );
    else if ( descfld_ )
	setHAlignObj( descfld_ );
    else
	setHAlignObj( colbut_ );
}


bool uiColorInput::doDraw() const
{
    return dodrawbox_ ? dodrawbox_->isChecked() : true;
}


void uiColorInput::setDoDraw( bool yn )
{
    if ( dodrawbox_ )
    {
	dodrawbox_->setChecked( yn );
	colbut_->setSensitive( yn );
    }
}


void uiColorInput::dodrawSel( CallBacker* )
{
    colbut_->setSensitive( dodrawbox_->isChecked() );
    if ( descfld_ )
	descfld_->setSensitive( dodrawbox_->isChecked() );
    if ( transpfld_ )
	transpfld_->setSensitive( dodrawbox_->isChecked() );
    doDrawChanged.trigger();
}


#define mSetTranspFromFld(col) \
    if ( transpfld_ ) \
    { \
	const float t = transpfld_->getFValue() * 2.55; \
	col.setTransparency( mRounded(unsigned char,t) ); \
    }


void uiColorInput::selCol( CallBacker* )
{
    OD::Color newcol = color_;
    const OD::Color oldcol = color_;
    selectColor( newcol, this, dlgtxt_, selwithtransp_ );
    mSetTranspFromFld(newcol);

    if ( oldcol != newcol )
    {
	setColor( newcol );
	colorChanged.trigger();
    }
}


void uiColorInput::descSel( CallBacker* )
{
    const int selidx = descfld_ ? descfld_->currentItem() : -1;
    if ( selidx < 0 ) return;

    OD::Color newcol( OD::Color::descriptionCenters()[selidx] );
    mSetTranspFromFld(newcol);
    setColor( newcol );
    colorChanged.trigger();
}


void uiColorInput::transpChg( CallBacker* )
{
    const int oldt = color_.t();
    mSetTranspFromFld( color_ );
    const int newt = color_.t();
    if ( oldt != newt )
	colorChanged.trigger();
}


void uiColorInput::setColor( const OD::Color& col )
{
    color_ = col;

    uiPixmap pm( colbut_->prefHNrPics()-10, colbut_->prefVNrPics()-10 );
    pm.fill( color_ );
    colbut_->setPixmap( pm );
    if ( transpfld_ )
    {
	const float perc = col.t() / 2.55;
	transpfld_->setValue( mNINT32(perc) );
    }

    if ( descfld_ )
    {
	NotifyStopper ns( descfld_->selectionChanged );
	const char* desc = color_.getDescription();
	if ( *desc == '~' ) desc++;
	descfld_->setText( desc );
    }
}


void uiColorInput::setLblText( const uiString& txt )
{
    if ( lbl_ ) lbl_->setText( txt );
}
