/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifont.cc,v 1.24 2008-11-25 15:35:24 cvsbert Exp $";

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

bool uiFontList::inited = false;
ObjectSet<uiFont> uiFontList::fonts;
static const char* fDefKey = "Font.def";


uiFont::uiFont( const char* key, const char* fam, int ps, FontData::Weight w,
		bool it )
	: mQtThing(new QFont(
			QString((fam && *fam) ? fam : "helvetica"),
			ps > 1 ? ps : 12 ,
                        FontData::numWeight(w),it))
	, mQFontMetrics( *new QFontMetrics( *mQtThing ))
	, key_( key )
{}


uiFont::uiFont( const char* key, FontData fdat )
	: mQtThing( new QFont(
		    QString( fdat.family() && *fdat.family()
				? fdat.family() : "helvetica" ),
		    fdat.pointSize() > 1 ? fdat.pointSize() : 12,
		    FontData::numWeight(fdat.weight()),
		    fdat.isItalic()))  
	, mQFontMetrics( *new QFontMetrics( *mQtThing ))
	, key_( key )
{}


uiFont::uiFont( const uiFont& afont )
	: mQtThing( new QFont( *afont.mQtThing ) )
	, mQFontMetrics( *new QFontMetrics( *mQtThing ))
	, key_( afont.key_ )
{} 


uiFont::~uiFont()
{
    delete mQtThing;
    delete &mQFontMetrics;
}


uiFont& uiFont::operator=( const uiFont& tf )
{
    if ( &tf != this )
    {
        setFontData( tf.fontData() );
    }
    return *this;
}


FontData uiFont::fontData() const
{
    return FontData( mQtThing->pointSize(), mQtThing->family(),
		     FontData::enumWeight(mQtThing->weight()),
		     mQtThing->italic() );
}


void uiFont::setFontData( const FontData& fData )
{
    mQtThing->setFamily( fData.family() );
    mQtThing->setPointSize( fData.pointSize() );
    mQtThing->setWeight( fData.weight() );
    mQtThing->setItalic( fData.isItalic() );
    updateMetrics();
}


void uiFont::updateMetrics()
{
    mQFontMetrics = QFontMetrics( *mQtThing );
}


int uiFont::height() const
{
    return mQFontMetrics.lineSpacing() + 2;
}


int uiFont::maxWidth() const
{
    return mQFontMetrics.maxWidth();
}


int uiFont::avgWidth() const
{
    return mQFontMetrics.width(QChar('x'));
}


int uiFont::width(const char* str) const
{
    return mQFontMetrics.width( QString( str ));
}


//! the inter-line spacing
int uiFont::leading() const
{
    return mQFontMetrics.leading();
}


//! the distance from the base line to the uppermost line with pixels.
int uiFont::ascent() const
{
    return mQFontMetrics.ascent();
}


bool select( uiFont& fnt, uiParent* parnt, const char* nm )
{
    bool ok;
  
    QFont fontNew;
    fontNew = QFontDialog::getFont( &ok, fnt.qFont(), 
				    parnt ? parnt->pbody()->qwidget() : 0, nm );
    if( ok ) 
    { 
	*fnt.mQtThing = fontNew;
        QFontMetrics metr( *fnt.mQtThing );
	fnt.mQFontMetrics = metr;
    }
    return ok;
}


//----------------------------------------------------------------------------

uiFont& uiFontList::add( const char* key, const char* family, int pointSize,
                         FontData::Weight weight, bool isItalic )
{
    return add( key, FontData( pointSize, family, weight, isItalic) );
}


uiFont& uiFontList::add( const char* key, const FontData& fdat )
{
    return gtFont( key, &fdat );
}


uiFont& uiFontList::get( const char* key )
{
    return gtFont( key, 0 );
}


uiFont& uiFontList::getFromQfnt( QFont* qf )
{
    return gtFont( 0, 0, qf );
}


int uiFontList::nrKeys()
{
    return fonts.size(); 
}


const char* uiFontList::key( int idx )
{
    return (const char*)fonts[idx]->key();
}


void uiFontList::listKeys( BufferStringSet& ids )
{
    initialise();

    for ( int idx=0; idx<fonts.size(); idx++ )
	ids.add( (const char*)fonts[idx]->key() );
}


uiFont& uiFontList::gtFont( const char* key, const FontData* fdat,
			    const QFont* qf )
{
    initialise();
    if ( (!key || !*key) && !qf ) return *fonts[0]; 

    for ( int idx=0; idx<fonts.size(); idx++ )
    {
	uiFont* fnt = fonts[ idx ];
	if ( key && !strcmp(fnt->key(),key) ) 
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
	return *fonts[0];
    else
    {
	uiFont* nwFont = new uiFont( key, *fdat );
	fonts += nwFont;
	return *nwFont;
    }
}


void uiFontList::initialise()
{
    if ( inited ) return;
    inited = true;
    use( Settings::common() );
}


void uiFontList::use( const Settings& settings )
{
    initialise();
    IOPar* fontpar = settings.subselect( fDefKey );
    if ( fontpar && fontpar->isEmpty() ) { delete fontpar; fontpar = 0; }

    bool haveguessed = false;
    int ikey=0;
    while ( const char* key = FontData::defaultKeys()[ikey++] )
    {
	const char* res = fontpar ? (*fontpar)[key] : 0;
	if ( res && *res )
	    add( key, FontData(res) );
	else if ( strcmp(key,"Fixed width") )
	    { addOldGuess( settings, key, ikey ); haveguessed = true; }
	else
	{
	    FontData fd = get(FontData::defaultKeys()[0]).fontData();
	    add( key, FontData( fd.pointSize(), "Courier", fd.weight(),
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
	    while( const char* key = FontData::defaultKeys()[ikey++] )
		if ( !strcmp(key,parkey) ) { isstd = true; break; }

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
    for ( int idx=0; idx<fonts.size(); idx++ )
    {
	uiFont& fnt = *fonts[idx];
	fnt.fontData().putTo( fdbuf );
	settings.set( IOPar::compKey(fDefKey,fnt.key()), fdbuf );
    }
    settings.write();
}


uiSetFonts::uiSetFonts( uiParent* p, const char* nm )
	: uiDialog(p,uiDialog::Setup("Fonts",nm,"0.2.2").mainwidgcentered(true))
{
    setCancelText( "" );
    uiFontList::initialise();
    const ObjectSet<uiFont>& fonts = uiFontList::fonts;
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

    if ( select(*uiFontList::fonts[idx],sender->parent()) )
    {
	uiFontList::update( Settings::common() );
	if ( !idx ) uiMain::theMain().setFont( uiFontList::get(), true );
    }
}





uiSelFonts::uiSelFonts( uiParent* p, const char* nm, const char* winid )
	: uiDialog(p,uiDialog::Setup("Fonts",nm,winid))
{
    uiFontList::listKeys( ids );
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

    return uiFontList::key(0);
}


void uiFontList::addOldGuess( const Settings& settings,
			      const char* key, int idx )
{
    const char* fontface = settings["Font face"];
    bool boldfont = true; settings.getYN( "Bold font", boldfont );

    int fontsz = FontData::defaultPointSize() * 10;
    if ( !strcmp(key,FontData::defaultKeys()[0]) )
	settings.get( "Dialog font size", fontsz );
    else if ( !strcmp(key,FontData::defaultKeys()[3]) )
	settings.get( "Graphics large font size", fontsz );
    else
	settings.get( "Graphics small font size", fontsz );

    add( key, FontData( fontsz/10, fontface,
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
