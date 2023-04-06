#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "fontdata.h"
#include "uistring.h"

class uiFont;
class uiParent;
class BufferStringSet;
class Settings;
mFDQtclass(QFont)
mFDQtclass(QFontMetrics)


mGlobal(uiBase) bool selectFont(uiFont&,uiParent* =0,
			const uiString& title=uiString::emptyString(),
			bool scalableonly=false);
/*!< \brief pops a selector box to select a new font
     \return true if new font selected
*/

mGlobal(uiBase) bool selectFont(FontData&,uiParent* =0,
			const uiString& title = uiString::emptyString(),
			bool scalableonly=false);
/*!< \brief pops a selector box to select a new font
     \return true if new font selected
*/


mExpClass(uiBase) uiFont : public CallBacker
{
friend class	uiFontList;

public:
    virtual		~uiFont();

    uiFont&		operator=(const uiFont&);

    FontData		fontData() const ;
    void		setFontData(const FontData&);
			//!< Updates internal QFont and QFontMetrics.
    static void		setFontData(mQtclass(QFont)&,const FontData&);
    static void		getFontData(FontData&,const mQtclass(QFont)&);
    static mQtclass(QFont)* createQFont(const FontData&);

    inline const mQtclass(QFont&)	qFont() const { return *qfont_; }

    int			height() const;
    int			leading() const;
    int			maxWidth() const;
    int			avgWidth() const;
    int			width(const uiString&) const;
    int			ascent() const;
    int			descent() const;

    const char*		key() const		{ return key_; }
    Notifier<uiFont>	changed;

protected:
			//! uiFont must be created through the uiFontList
			uiFont(const char* ky, const char* family,
				int ps=FontData::defaultPointSize(),
				FontData::Weight w=FontData::defaultWeight(),
				bool it=FontData::defaultItalic());
			uiFont(const char* ky,FontData fd=FontData());
			uiFont(const uiFont&);

    // don't change order of these attributes!
    mQtclass(QFont*)		qfont_;
    mQtclass(QFontMetrics&)	qfontmetrics_;

    BufferString		key_;

    void			updateMetrics();
};


mExpClass(uiBase) uiFontList : public CallBacker
{
friend class	uiFontSettingsGroup;

public:
			uiFontList();
			~uiFontList();
			mOD_DisableCopy(uiFontList)

    static uiFontList&	getInst();

    int			nrKeys();
    const char*		key(int);
    void		listKeys(BufferStringSet&);

    const ObjectSet<uiFont>&	fonts() const	{ return fonts_; }
    ObjectSet<uiFont>&	fonts()			{ return fonts_; }

    uiFont&		get(const char* ky=0);
    uiFont&		get(FontData::StdSz);
    uiFont&		getFromQfnt(mQtclass(QFont*));

    uiFont&		add(const char* ky,const FontData&);
    uiFont&		add(const char* ky,
			    const char* f=FontData::defaultFamily(),
			    int ptsz=FontData::defaultPointSize(),
			    FontData::Weight w=FontData::defaultWeight(),
			    bool it=FontData::defaultItalic());

    void		update(Settings&);

protected:

    ObjectSet<uiFont>	fonts_;
    void		initialize();
    uiFont&		gtFont(const char*,const FontData* =0,
			       const mQtclass(QFont*) =0 );

    void		use(const Settings&);

private:

    bool		inited_		= false;

    void		addOldGuess(const IOPar&,const char*);
    void		removeOldEntries(Settings&);
    void		setDefaults();

};

#define FontList    uiFontList::getInst
