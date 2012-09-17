/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink / Bril
 Date:          22/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicolor.cc,v 1.43 2012/07/10 13:06:04 cvskris Exp $";

#include "uicolor.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uispinbox.h"
#include "uibody.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimainwin.h"
#include "pixmap.h"
#include "uiparentbody.h"

#include <QColorDialog>
#include <QLabel>
#include <QApplication>


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


bool selectColor( Color& col, uiParent* parnt, const char* nm, bool withtransp )
{
    QWidget* qparent = parnt ? parnt->pbody()->qwidget() : 0;
    if ( !nm || !*nm ) nm = "Select color";

    const char* wintitle = uiMainWin::uniqueWinTitle( nm );
    const int refnr = beginCmdRecEvent( wintitle );

    QColorDialog qdlg( QColor(col.r(),col.g(),col.b(),col.t()), qparent );
    qdlg.setWindowTitle( QString(wintitle) );
    if ( withtransp )
    {
	qdlg.setOption( QColorDialog::ShowAlphaChannel );
	QList<QLabel*> lbllst = qdlg.findChildren<QLabel*>("");
	bool found = false;
	foreach(QLabel* qlbl,lbllst)
	{
	    if ( qlbl->text() == "A&lpha channel:" )
		{ qlbl->setText( "&Transparency:" ); found = true; break; }
	}
	if ( !found )
	    pFreeFnErrMsg("Implement support for label change in this Qt ver",
			    "selectColor" );
    }

    const bool ok = qdlg.exec() == QDialog::Accepted;

    if ( ok )
    {
	QColor newcol = qdlg.selectedColor();
	col.set( newcol.red(), newcol.green(), newcol.blue(),
		 withtransp ? newcol.alpha() : col.t() );
    }

    endCmdRecEvent( refnr, ok, col, withtransp );
    return ok;
}


void setExternalColor( const Color& col )
{
     QWidget* amw = qApp->activeModalWidget();
     QColorDialog* qcd = dynamic_cast<QColorDialog*>( amw );
     if ( qcd )
	 qcd->setCurrentColor( QColor(col.r(),col.g(),col.b(),col.t()) );
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
	dodrawbox_->setChecked( true );
	dodrawbox_->activated.notify( mCB(this,uiColorInput,dodrawSel) );
    }
    colbut_ = new uiPushButton( this, "", false );
    colbut_->setName( nm && *nm ? nm :
	    ( s.lbltxt_ && *s.lbltxt_ ? s.lbltxt_.buf() : "Color") );
    colbut_->activated.notify( mCB(this,uiColorInput,selCol) );
    if ( dodrawbox_ )
	colbut_->attach( rightOf, dodrawbox_ );
    
    if ( !dodrawbox_ && s.lbltxt_ && *s.lbltxt_)
	lbl_ = new uiLabel( this, s.lbltxt_, colbut_ );

    uiLabeledSpinBox* lsb = 0;
    if ( s.transp_ == Setup::Separate )
    {
	lsb = new uiLabeledSpinBox( this, "Transp", 0 );
	lsb->attach( rightOf, colbut_ );
	transpfld_ = lsb->box();
	transpfld_->setSuffix( "%" );
	transpfld_->setInterval( 0, 100, 1 );
	transpfld_->setHSzPol( uiObject::Small );
	transpfld_->valueChanged.notify( mCB(this,uiColorInput,transpChg) );
    }
    if ( s.withdesc_ )
    {
	descfld_ = new uiComboBox( this, Color::descriptions(),
				    "Color description" );
	descfld_->setHSzPol( uiObject::Medium );
	if ( lsb )
	    descfld_->attach( rightOf, lsb );
	else
	    descfld_->attach( rightOf, colbut_ );
	descfld_->selectionChanged.notify( mCB(this,uiColorInput,descSel) );
    }

    setColor( color_ ); 
    if ( lbl_ )
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
	const float t = transpfld_->getValue() * 2.55; \
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
    Color newcol( Color::descriptionCenters()[selidx] );
    mSetTranspFromFld(newcol);
    setColor( newcol );
    colorChanged.trigger();
}


void uiColorInput::transpChg( CallBacker* )
{
    mSetTranspFromFld( color_ );
}


void uiColorInput::setColor( const Color& col )
{
    color_ = col;

    ioPixmap pm( colbut_->prefHNrPics()-10, colbut_->prefVNrPics()-10 );
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


void uiColorInput::setLblText( const char* txt )
{
    if ( lbl_ ) lbl_->setText( txt );
}
