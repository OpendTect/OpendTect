/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uifont.cc,v 1.5 2001-05-04 10:09:00 windev Exp $
________________________________________________________________________

-*/

#include "uifontsel.h"
#include "uifont.h"
#include "uiobj.h"
#include "uimain.h"
#include "settings.h"
#include "uidset.h"
#include "uidialog.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"

#include "errh.h"

#include <qfont.h>
#include <qfontdialog.h> 
#include <qfontmetrics.h> 

bool uiFontList::inited = false;
ObjectSet<uiFont> uiFontList::fonts;
static const char* sKey = "Font.def";


uiFont::uiFont( const char* key, const char* fam, int ps, FontData::Weight w,
		bool it )
	: mQtThing(new QFont(QString(fam),ps,FontData::numWeight(w),it))
	, mQFontMetrics( *new QFontMetrics( *mQtThing ))
	, key_( key )
{
}


uiFont::uiFont( const char* key, FontData fdat )
	: mQtThing( new QFont(QString(fdat.family()),
		    fdat.pointSize(),FontData::numWeight(fdat.weight()),
		    fdat.isItalic()))  
	, mQFontMetrics( *new QFontMetrics( *mQtThing ))
	, key_( key )
{
}


uiFont::uiFont( const uiFont& afont )
	: mQtThing( new QFont( *afont.mQtThing ) )
	, mQFontMetrics( *new QFontMetrics( *mQtThing ))
	, key_( afont.key_ )
{
}


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
    return mQFontMetrics.width('x');
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


bool select( uiFont& fnt, uiObject* parnt, const char* nm )
{
    bool ok;
  
    QFont fontNew;
    fontNew = QFontDialog::getFont( &ok, fnt.qFont(), 
				    parnt ? &parnt->qWidget() : 0, nm );
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


int uiFontList::nrKeys()
{
    return fonts.size(); 
}


const char* uiFontList::key( int idx )
{
    return (const char*)fonts[idx]->key();
}


void uiFontList::listKeys( ObjectSet<const char>& strs )
{
    initialise();

    for ( int idx=0; idx<fonts.size(); idx++ )
	strs += (const char*)fonts[idx]->key();
}


void uiFontList::listKeys( UserIDSet& ids )
{
    initialise();

    for ( int idx=0; idx<fonts.size(); idx++ )
	ids.add( (const char*)fonts[idx]->key() );
}


uiFont& uiFontList::gtFont( const char* key, const FontData* fdat )
{
    initialise();
    if( !key || !*key ) return *fonts[0]; 

    for ( int idx=0; idx<fonts.size(); idx++ )
    {
	uiFont* fnt = fonts[ idx ];
	if ( !strcmp(fnt->key(),key) ) 
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
    IOPar* fontpar = settings.subselect( sKey );
    if ( fontpar && fontpar->size() == 0 ) { delete fontpar; fontpar = 0; }

    bool haveguessed = false;
    int ikey=0;
    while( const char* key = FontData::defaultKeys()[ikey++] )
    {
	const char* res = fontpar ? (*fontpar)[key] : 0;
	if ( res && *res )
	    add( key, FontData(res) );
	else
	    { addOldGuess( settings, key, ikey ); haveguessed = true; }
    }

    if ( haveguessed )
    {
	Settings& s = const_cast<Settings&>(settings);
	/* For next release:
	removeOldEntries( s );
	*/
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
	settings.set( IOPar::compKey(sKey,fnt.key()), fdbuf );
    }
    settings.write();
}


uiSetFonts::uiSetFonts( uiObject* p, const char* nm )
	: uiDialog(p,nm)
{
    cnclText = "";
    uiFontList::initialise();
    const ObjectSet<uiFont>& fonts = uiFontList::fonts;
    for ( int idx=0; idx<fonts.size(); idx++ )
    {
	uiButton* but = new uiPushButton( this, (const char*)fonts[idx]->key());
        but->notify( mCB(this,uiSetFonts,butPushed) );
	if( idx ) but->attach( centeredBelow, buttons[idx-1]);
	buttons += but;
    }
}



void uiSetFonts::butPushed( CallBacker* obj )
{
    uiButton* sender = dynamic_cast<uiButton*>(obj);
    int idx = buttons.indexOf( sender );
    if ( idx < 0 ) { pErrMsg("idx < 0. Why?"); return; }

    if ( select(*uiFontList::fonts[idx],sender) )
    {
	uiFontList::update( Settings::common() );
	if ( !idx ) uiMain::theMain().setFont( uiFontList::get(0), true );
    }
}





uiSelFonts::uiSelFonts( uiObject* p, const char* nm )
	: uiDialog(p,nm)
	, ids(*new UserIDSet)
{
    uiFontList::listKeys( ids );
}


uiSelFonts::~uiSelFonts()
{
    ids.deepErase();
    delete &ids;
}


void uiSelFonts::add( const char* nm, const char* stdfontkey )
{
    ids.setName( nm );
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, ids );
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
	    return sels[idx]->box()->getText();
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
