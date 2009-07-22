/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink / Bril
 Date:          22/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicolor.cc,v 1.26 2009-07-22 16:01:38 cvsbert Exp $";

#include "uicolor.h"
#include "uibutton.h"
#include "uibody.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimainwin.h"
#include "pixmap.h"
#include "uiparentbody.h"

#include <QColorDialog>


static int beginCmdRecEvent()
{
    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( !carrier )
	return -1;

    return carrier->beginCmdRecEvent( -1, "QColorDlg" );
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
	carrier->endCmdRecEvent( -1, refnr, msg );
}


bool selectColor( Color& col, uiParent* parnt, const char* nm, bool withtransp )
{
    bool ok;
    QRgb rgb;
    const int refnr = beginCmdRecEvent();

    if ( withtransp )
    {
	rgb = QColorDialog::getRgba( (QRgb)col.rgb(), &ok, 
		      parnt ? parnt->pbody()->qwidget() : 0 );
    }
    else
    {
	QColor newcol = QColorDialog::getColor( QColor((QRgb) col.rgb()) , 
		      parnt ? parnt->pbody()->qwidget() : 0 );

	ok = newcol.isValid();
	rgb = newcol.rgb(); 
    }

    if ( externalcolor )		// Command driver interference
    {
	col = *externalcolor;
	if ( !withtransp )
	    col.setTransparency( 0 );

	delete externalcolor;
	externalcolor = 0;
	return true;
    }

    if ( ok )
    {
	col.setRgb( rgb );
	if ( !withtransp )
	    col.setTransparency( 0 );
    }

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
	, withalpha_(s.withalpha_)
	, colorchanged(this)
	, dodrawchanged(this)
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
    dodrawchanged.trigger();
}


void uiColorInput::selCol( CallBacker* )
{
    Color newcol = color_;
    const Color oldcol = color_;
    selectColor( newcol, this, dlgtxt_, withalpha_ );
    if ( oldcol != newcol )
    {
	setColor( newcol );
	colorchanged.trigger();
    }
}


void uiColorInput::setColor( const Color& col )
{
    color_ = col;

    ioPixmap pm( colbut_->prefHNrPics()-10, colbut_->prefVNrPics()-10 );
    pm.fill( color_ );
    colbut_->setPixmap( pm );
}


void uiColorInput::setLblText( const char* txt )
{
    if ( uilbl_ )
	uilbl_->setText( txt );
}
