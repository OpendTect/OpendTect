/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink / Bert
 Date:          22/05/2000
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
    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( !carrier )
	return -1;

    BufferString msg( "QColorDlg " );
    msg += windowtitle;

    return carrier->beginCmdRecEvent( mGlobalQColorDlgCmdRecId, msg );
}

static void endCmdRecEvent( int refnr, bool ok, const Color& col,
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

    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( carrier )
	carrier->endCmdRecEvent( mGlobalQColorDlgCmdRecId, refnr, msg );
}


uiString uiColorInput::sSelColor()
{ return uiStrings::phrSelect( uiStrings::sColor() ); }


bool selectColor( Color& col, uiParent* parnt,
                uiString txt, bool withtransp )
{ return uiColorInput::selectColor( col, parnt, txt, withtransp ); }



bool uiColorInput::selectColor( Color& col, uiParent* parnt,
                                uiString txt, bool withtransp )
{
    QWidget* qparent = parnt ? parnt->pbody()->qwidget() : 0;
    if ( txt.isEmpty() ) txt = uiColorInput::sSelColor();

    BufferString addendum;
    const uiString wintitle = uiMainWin::uniqueWinTitle( txt, 0, &addendum );
    BufferString utfwintitle( toString(txt), addendum );
    const int refnr = ::beginCmdRecEvent( utfwintitle );

    QColorDialog qdlg( QColor(col.r(),col.g(), col.b(),col.t()), qparent );
    qdlg.setWindowTitle( toQString(wintitle) );
    if ( withtransp )
    {
        qdlg.setOption( QColorDialog::ShowAlphaChannel );
        qdlg.setOption( QColorDialog::DontUseNativeDialog );
	QList<QLabel*> lbllst = qdlg.findChildren<QLabel*>("");
	bool found = false;
	foreach( QLabel* qlbl, lbllst )
	{
	    if ( qlbl->text().contains( "pha channel:" ) )
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


void setExternalColor( const Color& col )
{
     QWidget* amw = qApp->activeModalWidget();
     QColorDialog* qcd = dynamic_cast<QColorDialog*>( amw );
     if ( qcd )
	 qcd->setCurrentColor( QColor(col.r(),col.g(),col.b(),
						col.t()) );
}


uiColorInput::uiColorInput( uiParent* p, const Setup& setup, const char* nm )
	: uiGroup(p,"Color input")
	, color_(setup.color_)
	, dlgtxt_(setup.dlgtitle_)
	, dodrawbox_(0)
	, lbl_(0)
	, transpfld_(0)
	, descfld_(0)
	, selwithtransp_(setup.transp_ == Setup::InSelector)
	, colorChanged(this)
	, doDrawChanged(this)
{
    if ( setup.withcheck_ )
    {
	dodrawbox_ = new uiCheckBox( this, setup.lbltxt_);
	dodrawbox_->setChecked( true );
	dodrawbox_->activated.notify( mCB(this,uiColorInput,dodrawSel) );
    }

    colbut_ = new uiPushButton( this,uiString::empty(), false );
    BufferString colbutnm( nm );
    if ( colbutnm.isEmpty() )
	colbutnm = toString( setup.lbltxt_ );
    if ( colbutnm.isEmpty() )
	colbutnm = "Color";
    colbut_->setName( colbutnm );
    colbut_->activated.notify( mCB(this,uiColorInput,selCol) );
    if ( dodrawbox_ )
	colbut_->attach( rightOf, dodrawbox_ );

    if ( !dodrawbox_ && !setup.lbltxt_.isEmpty() )
	lbl_ = new uiLabel( this, setup.lbltxt_, colbut_ );

    uiLabeledSpinBox* lsb = 0;
    if ( setup.transp_ == Setup::Separate )
    {
	lsb = new uiLabeledSpinBox( this, uiStrings::sTransparency(), 0 );
	lsb->attach( rightOf, colbut_ );
	transpfld_ = lsb->box();
	transpfld_->setSuffix( toUiString("%") );
	transpfld_->setInterval( 0, 100, 1 );
	transpfld_->setHSzPol( uiObject::Small );
	transpfld_->valueChanged.notify( mCB(this,uiColorInput,transpChg) );
    }
    if ( setup.withdesc_ )
    {
	BufferStringSet coldescs; Color::getDescriptions( coldescs );
	descfld_ = new uiComboBox( this, coldescs, "Color description" );
	descfld_->setHSzPol( uiObject::Medium );
	Color::getDescriptionCenters( desccolors_ );
	for ( int idx=0; idx<desccolors_.size(); idx++ )
	{
	    uiPixmap pm( 15, 10 );
	    pm.fill( desccolors_[idx] );
	    descfld_->setPixmap( idx, pm );
	}

	if ( lsb )
	    descfld_->attach( rightOf, lsb );
	else
	    descfld_->attach( rightOf, colbut_ );
	descfld_->selectionChanged.notify( mCB(this,uiColorInput,descSel) );
	descfld_->setHSzPol( uiObject::MedMax );
    }

    if ( lbl_ || dodrawbox_ )
	setHAlignObj( colbut_ );
    else if ( transpfld_ )
	setHAlignObj( transpfld_ );
    else if ( descfld_ )
	setHAlignObj( descfld_ );
    else
	setHAlignObj( colbut_ );

    postFinalise().notify( mCB(this,uiColorInput,initFlds) );
}


void uiColorInput::initFlds( CallBacker* )
{
    setColor( color_ );
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
	const float t = transpfld_->getFValue() * 2.55f; \
	col.setTransparency( mRounded(unsigned char,t) ); \
    }


void uiColorInput::selCol( CallBacker* )
{
    Color newcol = color_;
    const Color oldcol = color_;
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

    Color newcol( desccolors_[selidx] );
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


void uiColorInput::setColor( const Color& col )
{
    color_ = col;

    uiPixmap pm( colbut_->width(), colbut_->height() );
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
	const BufferString coldesc = color_.getDescription();
	const char* desc = coldesc.buf();
	if ( *desc == '~' )
	    desc++;
	descfld_->setText( desc );
    }
}


void uiColorInput::setLblText( const uiString& txt )
{
    if ( lbl_ ) lbl_->setText( txt );
}
