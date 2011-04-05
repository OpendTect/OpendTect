/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink / Bril
 Date:          22/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicolor.cc,v 1.35 2011-04-05 14:04:33 cvsbert Exp $";

#include "uicolor.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uibody.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimainwin.h"
#include "pixmap.h"
#include "uiparentbody.h"

#include <QColorDialog>


#define mGlobalQColorDlgCmdRecId 1

static int beginCmdRecEvent()
{
    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( !carrier )
	return -1;

    const char* windowtitle = "Select color";
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
    const int refnr = beginCmdRecEvent();

    QColor oldcol( col.r(), col.g(), col.b(), 255-col.t() );
    QWidget* qparent = parnt ? parnt->pbody()->qwidget() : 0;
#if QT_VERSION >= 0x040500
    QColorDialog::ColorDialogOptions options = 0;
    if ( withtransp ) options = QColorDialog::ShowAlphaChannel;
    QColor newcol = QColorDialog::getColor( oldcol, qparent, nm, options );
#else
    QColor newcol;
    if ( withtransp )
    {
	//note that equal operator does not work
	newcol.setRgba( QColorDialog::getRgba(oldcol.rgba(),0,qparent) );
    }
    else
	newcol = QColorDialog::getColor( oldcol, qparent );
#endif

    if ( externalcolor )		// Command driver interference
    {
	col = *externalcolor;
	if ( !withtransp )
	    col.setTransparency( 0 );

	delete externalcolor;
	externalcolor = 0;
	return true;
    }

    const bool ok = newcol.isValid();
    if ( ok )
	col.set( newcol.red(), newcol.green(), newcol.blue(),
		 withtransp ? 255-newcol.alpha() : 0 );

    endCmdRecEvent( refnr, ok, col, withtransp );

    return ok;
}


void setExternalColor( const Color& col )
{
    
    if ( !externalcolor )
	externalcolor = new Color( col );
    else
	*externalcolor = col;
}


uiColorInput::uiColorInput( uiParent* p, const Setup& s, const char* nm )
	: uiGroup(p,"Color input")
	, color_(s.color_)
	, dlgtxt_(s.dlgtitle_)
	, dodrawbox_(0)
	, uilbl_(0)
	, descfld_(0)
	, withalpha_(s.withalpha_)
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
	uilbl_ = new uiLabel( this, s.lbltxt_, colbut_ );

    if ( s.withdesc_ )
    {
	descfld_ = new uiComboBox( this, Color::descriptions(),
				    "Color description" );
	descfld_->setHSzPol( uiObject::Medium );
	descfld_->attach( rightOf, colbut_ );
	descfld_->selectionChanged.notify( mCB(this,uiColorInput,descSel) );
    }

    setColor( color_ ); 
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
    doDrawChanged.trigger();
}


void uiColorInput::selCol( CallBacker* )
{
    Color newcol = color_;
    const Color oldcol = color_;
    selectColor( newcol, this, dlgtxt_, withalpha_ );
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
    setColor( Color::descriptionCenters()[selidx] );
    colorChanged.trigger();
}


void uiColorInput::setColor( const Color& col )
{
    color_ = col;

    ioPixmap pm( colbut_->prefHNrPics()-10, colbut_->prefVNrPics()-10 );
    pm.fill( color_ );
    colbut_->setPixmap( pm );
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
    if ( uilbl_ )
	uilbl_->setText( txt );
}
