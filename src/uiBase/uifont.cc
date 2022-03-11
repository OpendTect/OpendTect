/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          22/05/2000
________________________________________________________________________

-*/

#include "uifont.h"
#include "q_uiimpl.h"

#include "uimain.h"
#include "uiparent.h"
#include "bufstringset.h"
#include "od_helpids.h"
#include "settings.h"

#include "q_uiimpl.h"

#include <QFont>
#include <QFontDialog>
#include <QFontMetrics>

mUseQtnamespace

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    #define mGetTextWidth(qfm,textstring) qfm.horizontalAdvance( textstring )
#else
    #define mGetTextWidth(qfm,textstring) qfm.width( textstring )
#endif

static const char* fDefKey = "Font.def";

uiFont::uiFont( const char* ky, const char* fam, int ps, FontData::Weight w,
		bool it )
    : qfont_(new QFont(QString(fam && *fam ? fam : "arial"),
		       ps > 1 ? ps : 12, FontData::numWeight(w),it))
    , qfontmetrics_(*new QFontMetrics(*qfont_))
    , key_( ky )
    , changed(this)
{
}


uiFont::uiFont( const char* ky, FontData fd )
    : qfont_(createQFont(fd))
    , qfontmetrics_(*new QFontMetrics(*qfont_))
    , key_( ky )
    , changed(this)
{}


uiFont::uiFont( const uiFont& afont )
    : qfont_(new QFont(*afont.qfont_))
    , qfontmetrics_(*new QFontMetrics(*qfont_))
    , key_(afont.key_)
    , changed(this)
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
    FontData fd;
    getFontData( fd, *qfont_ );
    return fd;
}


void uiFont::setFontData( const FontData& fd )
{
    setFontData( *qfont_, fd );
    updateMetrics();
}


void uiFont::setFontData( mQtclass(QFont)& qfont, const FontData& fd )
{
    qfont = QFont( fd.family(),
		   fd.pointSize(),
		   FontData::numWeight(fd.weight()),
		   fd.isItalic() );
    QString stylenm = qfont.styleName();
    if ( stylenm.isEmpty() )
	qfont.setStyleName( fd.isItalic() ? "Italic" : "Regular" );
}


void uiFont::getFontData( FontData& fd, const mQtclass(QFont)& qfont )
{
    const BufferString fontfamily = qfont.family();
    fd = FontData( qfont.pointSize(), fontfamily,
		   FontData::enumWeight(qfont.weight()),
		   qfont.italic() );
}


void uiFont::updateMetrics()
{
    qfontmetrics_ = QFontMetrics( *qfont_ );
    changed.trigger();
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
    return mGetTextWidth(qfontmetrics_,QChar('x'));
}


int uiFont::width(const uiString& str) const
{
    return mGetTextWidth(qfontmetrics_,toQString(str));
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



// uiFontList

uiFontList::~uiFontList()
{
    deepErase( fonts_ );
}

uiFontList& uiFontList::getInst()
{
    mDefineStaticLocalObject( uiFontList, fl, );
    return fl;
}


uiFont& uiFontList::add( const char* ky, const char* family, int pointSize,
			 FontData::Weight weight, bool isItalic )
{
    return add( ky, FontData( pointSize, family, weight, isItalic) );
}


uiFont& uiFontList::add( const char* ky, const FontData& fd )
{
    return gtFont( ky, &fd );
}


uiFont& uiFontList::get( const char* ky )
{
    return gtFont( ky, 0 );
}


uiFont& uiFontList::get( FontData::StdSz std )
{
    return gtFont( FontData::key(std) );
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


uiFont& uiFontList::gtFont( const char* ky, const FontData* fd, const QFont* qf)
{
    initialise();
    if ( (!ky || !*ky) && !qf ) return *fonts_[0];

    for ( int idx=0; idx<fonts_.size(); idx++ )
    {
	uiFont* fnt = fonts_[ idx ];
	if ( ky && !strcmp(fnt->key(),ky) )
	{
	    if ( fd ) fnt->setFontData( *fd );
	    return *fnt;
	}
	if( qf && fnt->qFont() == *qf )
	{
	    if ( fd ) fnt->setFontData( *fd );
	    return *fnt;
	}
    }

    if ( !fd )
	return *fonts_[0];
    else
    {
	uiFont* nwFont = new uiFont( ky, *fd );
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


void uiFontList::setDefaults()
{
    FontData fd( 10, "Helvetica", FontData::Normal, false );
    if ( __ismac__ )
	fd.setPointSize( 12 );
    else if ( __islinux__ )
	fd.setFamily( "Nimbus Sans L" );
    else if ( __iswin__ )
	fd.setFamily( "Arial" );

    FontData fdsmall = fd; fdsmall.setPointSize( fd.pointSize()-2 );
    FontData fdlarge = fd; fdlarge.setPointSize( fd.pointSize()+2 );
    FontData fd3d = fd; fd3d.setPointSize( 16 );

    add( FontData::key(FontData::Control), fd );
    add( FontData::key(FontData::Graphics2D), fd );
    add( FontData::key(FontData::Graphics3D), fd3d );
    add( FontData::key(FontData::ControlSmall), fdsmall );
    add( FontData::key(FontData::ControlLarge), fdlarge );
    add( FontData::key(FontData::Graphics2DSmall), fdsmall );
    add( FontData::key(FontData::Graphics2DLarge), fdlarge );

    FontData fdfixed = fd; fdfixed.setFamily( "Courier" );
    add( FontData::key(FontData::Fixed), fdfixed );

    update( Settings::common() );
}


void uiFontList::use( const Settings& settings )
{
    PtrMan<IOPar> fontpar = settings.subselect( fDefKey );
    if ( !fontpar )
	return setDefaults();

    bool haveguessed = false;
    int ikey=0;
    while ( const char* ky = FontData::defaultKeys()[ikey++] )
    {
	const char* res = fontpar ? fontpar->find(ky) : 0;
	if ( res && *res )
	    add( ky, FontData(res) );
	else if ( fontpar && strcmp(ky,"Fixed width") )
	    { addOldGuess( *fontpar, ky ); haveguessed = true; }
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
    settings.write( false );
}


void uiFontList::addOldGuess( const IOPar& fontpar, const char* ky )
{
    const FixedString fontkey = ky;

    if ( fontkey == FontData::key(FontData::Graphics2D) )
    {
	const FixedString res = fontpar.find( "Graphics medium" );
	if ( !res.isEmpty() ) add( ky, FontData(res.buf()) );
    }
    else if ( fontkey == FontData::key(FontData::Graphics3D) )
    {
	const FixedString res =
		Settings::common().find( "dTect.Scene.Annotation font" );
	if ( !res.isEmpty() ) add( ky, FontData(res.buf()) );
    }
    else if ( fontkey == FontData::key(FontData::Graphics2DSmall) )
    {
	const FixedString res = fontpar.find( "Graphics small" );
	if ( !res.isEmpty() ) add( ky, FontData(res.buf()) );
    }
    else if ( fontkey == FontData::key(FontData::Graphics2DLarge) )
    {
	const FixedString res = fontpar.find( "Graphics large" );
	if ( !res.isEmpty() ) add( ky, FontData(res.buf()) );
    }
}


void uiFontList::removeOldEntries( Settings& settings )
{
    settings.removeWithKey( IOPar::compKey(fDefKey,"Graphics medium") );
    settings.removeWithKey( IOPar::compKey(fDefKey,"Graphics large") );
    settings.removeWithKey( IOPar::compKey(fDefKey,"Graphics small") );
    settings.removeWithKey( "dTect.Scene.Annotation font" );
}


mQtclass(QFont)* uiFont::createQFont( const FontData& fd )
{
    mQtclass(QFont)* res = new QFont;
    setFontData( *res, fd );
    return res;
}


// selectFont functions
static bool getFont( mQtclass(QFont)& qfontout,
		     const mQtclass(QFont)& qfontin,
		     uiParent* par, const uiString& nm )
{
    bool ok = false;
    qfontout = mQtclass(QFontDialog)::getFont( &ok, qfontin,
			par ? par->getWidget() : 0,
			toQString(nm) );
    return ok;
}


bool selectFont( uiFont& fnt, uiParent* par, const uiString& nm )
{
    mQtclass(QFont) qfont;
    if ( !getFont(qfont,fnt.qFont(),par,nm) )
	return false;

    FontData fd;
    uiFont::getFontData( fd, qfont );
    fnt.setFontData( fd );
    return true;
}


bool selectFont( FontData& fd, uiParent* par, const uiString& nm )
{
    mQtclass(QFont) qfont;
    uiFont::setFontData( qfont, fd );
    if ( !getFont(qfont,qfont,par,nm) )
	return false;

    uiFont::getFontData( fd, qfont );
    return true;
}
