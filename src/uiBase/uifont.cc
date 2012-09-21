/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          22/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uifontsel.h"
#include "uifont.h"
#include "uiobj.h"
#include "uimain.h"
#include "settings.h"
#include "uidialog.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uiparentbody.h"
#include "uilabel.h"
#include "bufstringset.h"

#include "errh.h"
#include "uibody.h"

#include <qfont.h>
#include <qfontdialog.h> 
#include <qfontmetrics.h> 

static const char* fDefKey = "Font.def";


uiFont::uiFont( const char* ky, const char* fam, int ps, FontData::Weight w,
		bool it )
	: qfont_(new QFont(QString( fam && *fam ? fam : "helvetica"),
			   ps > 1 ? ps : 12, FontData::numWeight(w),it))
	, qfontmetrics_(*new QFontMetrics(*qfont_))
	, key_( ky )
{}


uiFont::uiFont( const char* ky, FontData fdat )
	: qfont_( new QFont(
		    QString( fdat.family() && *fdat.family()
				? fdat.family() : "helvetica" ),
		    fdat.pointSize() > 1 ? fdat.pointSize() : 12,
		    FontData::numWeight(fdat.weight()),
		    fdat.isItalic()))  
	, qfontmetrics_(*new QFontMetrics(*qfont_))
	, key_( ky )
{}


uiFont::uiFont( const uiFont& afont )
	: qfont_(new QFont(*afont.qfont_))
	, qfontmetrics_(*new QFontMetrics(*qfont_))
	, key_(afont.key_)
{} 


uiFont::~uiFont()
{
    delete qfont_;
    delete &qfontmetrics_;
}


uiFont& uiFont::operator=( const uiFont& tf )
{
    if ( &tf != this )
        setFontData( tf.fontData() );
    return *this;
}


FontData uiFont::fontData() const
{
    return FontData( qfont_->pointSize(), mQStringToConstChar(qfont_->family()),
		     FontData::enumWeight(qfont_->weight()),
		     qfont_->italic() );
}


void uiFont::setFontData( const FontData& fData )
{
    setFontData( *qfont_, fData );
    updateMetrics();
}


void uiFont::setFontData( QFont& qfont, const FontData& fData )
{
    qfont.setFamily( fData.family() );
    qfont.setPointSize( fData.pointSize() );
    qfont.setWeight( fData.weight() );
    qfont.setItalic( fData.isItalic() );
}


void uiFont::updateMetrics()
{
    qfontmetrics_ = QFontMetrics( *qfont_ );
}


int uiFont::height() const
{
    return qfontmetrics_.lineSpacing() + 2;
}


int uiFont::maxWidth() const
{
    return qfontmetrics_.maxWidth();
}


int uiFont::avgWidth() const
{
    return qfontmetrics_.width(QChar('x'));
}


int uiFont::width(const char* str) const
{
    return qfontmetrics_.width( QString( str ));
}


//! the inter-line spacing
int uiFont::leading() const
{
    return qfontmetrics_.leading();
}


//! the distance from the base line to the uppermost line with pixels.
int uiFont::ascent() const
{
    return qfontmetrics_.ascent();
}


bool select( uiFont& fnt, uiParent* parnt, const char* nm )
{
    bool ok;
  
    QFont fontNew;
    fontNew = QFontDialog::getFont( &ok, fnt.qFont(), 
				    parnt ? parnt->pbody()->qwidget() : 0, nm );
    if( ok ) 
    { 
	*fnt.qfont_ = fontNew;
        QFontMetrics metr( *fnt.qfont_ );
	fnt.qfontmetrics_ = metr;
    }
    return ok;
}


//----------------------------------------------------------------------------

uiFontList::~uiFontList()
{
    deepErase( fonts_ );
}

uiFontList& uiFontList::getInst()
{
    static uiFontList* fl = 0;
    if ( !fl ) fl = new uiFontList;
    return *fl;
}


uiFont& uiFontList::add( const char* ky, const char* family, int pointSize,
                         FontData::Weight weight, bool isItalic )
{
    return add( ky, FontData( pointSize, family, weight, isItalic) );
}


uiFont& uiFontList::add( const char* ky, const FontData& fdat )
{
    return gtFont( ky, &fdat );
}


uiFont& uiFontList::get( const char* ky )
{
    return gtFont( ky, 0 );
}


uiFont& uiFontList::getFromQfnt( QFont* qf )
{
    return gtFont( 0, 0, qf );
}


int uiFontList::nrKeys()
{
    return fonts_.size(); 
}


const char* uiFontList::key( int idx )
{
    return (const char*)fonts_[idx]->key();
}


void uiFontList::listKeys( BufferStringSet& ids )
{
    initialise();

    for ( int idx=0; idx<fonts_.size(); idx++ )
	ids.add( (const char*)fonts_[idx]->key() );
}


uiFont& uiFontList::gtFont( const char* ky, const FontData* fdat,
			    const QFont* qf )
{
    initialise();
    if ( (!ky || !*ky) && !qf ) return *fonts_[0]; 

    for ( int idx=0; idx<fonts_.size(); idx++ )
    {
	uiFont* fnt = fonts_[ idx ];
	if ( ky && !strcmp(fnt->key(),ky) ) 
	{
	    if ( fdat ) fnt->setFontData( *fdat );
	    return *fnt; 
	}
	if( qf && fnt->qFont() == *qf )
	{
	    if ( fdat ) fnt->setFontData( *fdat );
	    return *fnt; 
	}
    }

    if ( !fdat )
	return *fonts_[0];
    else
    {
	uiFont* nwFont = new uiFont( ky, *fdat );
	fonts_ += nwFont;
	return *nwFont;
    }
}


void uiFontList::initialise()
{
    if ( inited_ ) return;
    inited_ = true;
    use( Settings::common() );
}


void uiFontList::use( const Settings& settings )
{
    initialise();
    IOPar* fontpar = settings.subselect( fDefKey );
    if ( fontpar && fontpar->isEmpty() ) { delete fontpar; fontpar = 0; }

    bool haveguessed = false;
    int ikey=0;
    while ( const char* ky = FontData::defaultKeys()[ikey++] )
    {
	const char* res = fontpar ? (*fontpar)[ky] : 0;
	if ( res && *res )
	    add( ky, FontData(res) );
	else if ( strcmp(ky,"Fixed width") )
	    { addOldGuess( settings, ky, ikey ); haveguessed = true; }
	else
	{
	    FontData fd = get(FontData::defaultKeys()[0]).fontData();
	    add( ky, FontData( fd.pointSize(), "Courier", fd.weight(),
				fd.isItalic() ) );
	}
    }

    if ( haveguessed )
    {
	Settings& s = const_cast<Settings&>(settings);
	removeOldEntries( s );
	update( s );
    }

    if ( fontpar )
    {
	for ( int ipar=0; ipar<fontpar->size(); ipar++ )
	{
	    const char* parkey = fontpar->getKey(ipar);
	    bool isstd = false;
	    ikey = 0;
	    while( const char* ky = FontData::defaultKeys()[ikey++] )
		if ( !strcmp(ky,parkey) ) { isstd = true; break; }

	    if ( !isstd )
		add( parkey, FontData( (*fontpar)[parkey] ) );
	}

	delete fontpar;
    }
}


void uiFontList::update( Settings& settings )
{
    initialise();
    BufferString fdbuf;
    for ( int idx=0; idx<fonts_.size(); idx++ )
    {
	uiFont& fnt = *fonts_[idx];
	fnt.fontData().putTo( fdbuf );
	settings.set( IOPar::compKey(fDefKey,fnt.key()), fdbuf );
    }
    settings.write();
}


uiSetFonts::uiSetFonts( uiParent* p, const char* nm )
	: uiDialog(p,uiDialog::Setup("Fonts",nm,"0.2.2").mainwidgcentered(true))
{
    setCancelText( "" );
    FontList().initialise();
    const ObjectSet<uiFont>& fonts = FontList().fonts();
    uiButtonGroup* butgrp = new uiButtonGroup( this, "" );
    butgrp->setPrefWidthInChar( 25 );
    for ( int idx=0; idx<fonts.size(); idx++ )
    {
	uiButton* but = new uiPushButton( butgrp, fonts[idx]->key(), false );
        but->activated.notify( mCB(this,uiSetFonts,butPushed) );
	buttons += but;
    }

    butgrp->attach( hCentered );
}



void uiSetFonts::butPushed( CallBacker* obj )
{
    mDynamicCastGet(uiButton*,sender,obj)
    int idx = buttons.indexOf( sender );
    if ( idx < 0 ) { pErrMsg("idx < 0. Why?"); return; }

    if ( select(*FontList().fonts()[idx],sender->parent()) )
    {
	FontList().update( Settings::common() );
	if ( !idx ) uiMain::theMain().setFont( FontList().get(), true );
    }
}





uiSelFonts::uiSelFonts( uiParent* p, const char* nm, const char* winid )
	: uiDialog(p,uiDialog::Setup("Fonts",nm,winid))
{
    FontList().listKeys( ids );
}


uiSelFonts::~uiSelFonts()
{
}


void uiSelFonts::add( const char* nm, const char* stdfontkey )
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, ids, nm );
    if ( sels.size() )
	lcb->attach( alignedBelow, sels[sels.size()-1] );
    lcb->box()->setCurrentItem( stdfontkey );
    sels += lcb;
}


const char* uiSelFonts::resultFor( const char* str )
{
    for ( int idx=0; idx<sels.size(); idx++ )
    {
	if ( sels[idx]->label()->name() == str )
	    return sels[idx]->box()->text();
    }

    return FontList().key(0);
}


void uiFontList::addOldGuess( const Settings& settings,
			      const char* ky, int idx )
{
    const char* fontface = settings["Font face"];
    bool boldfont = true; settings.getYN( "Bold font", boldfont );

    int fontsz = FontData::defaultPointSize() * 10;
    if ( !strcmp(ky,FontData::defaultKeys()[0]) )
	settings.get( "Dialog font size", fontsz );
    else if ( !strcmp(ky,FontData::defaultKeys()[3]) )
	settings.get( "Graphics large font size", fontsz );
    else
	settings.get( "Graphics small font size", fontsz );

    add( ky, FontData( fontsz/10, fontface,
			boldfont ? FontData::Bold : FontData::Normal ) );
}


void uiFontList::removeOldEntries( Settings& settings )
{
    settings.removeWithKey( "Font face" );
    settings.removeWithKey( "Bold font" );
    settings.removeWithKey( "Dialog font size" );
    settings.removeWithKey( "Graphics large font size" );
    settings.removeWithKey( "Graphics small font size" );
}
