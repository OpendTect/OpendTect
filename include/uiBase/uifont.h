#ifndef uifont_H
#define uifont_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uifont.h,v 1.7 2008-12-24 05:52:49 cvsnanne Exp $
________________________________________________________________________

-*/
#include "fontdata.h"

class QFont;
class QFontMetrics;
class Settings;
class uiParent;
class BufferStringSet;

class uiFont 
{			//!< font stuff that needs Qt.

    friend bool		select(uiFont&,uiParent*,const char*); 
    friend class	uiFontList;

protected:
			uiFont(const char* key, const char* family,
				int ps=FontData::defaultPointSize(),
				FontData::Weight w=FontData::defaultWeight(),
				bool it=FontData::defaultItalic());
			uiFont(const char* key,FontData fd=FontData());
			uiFont(const uiFont&);

public:
			//! uiFont must be created through the uiFontList

    virtual		~uiFont();
    uiFont&		 operator=(const uiFont&);
    
    FontData		fontData() const ;
    void		setFontData(const FontData&); 
                        //!< Updates internal QFont and QFontMetrics.

    inline const QFont&	qFont() const { return *qfont_; } 

    int			height() const;
    int			leading() const; 
    int 		maxWidth() const;
    int 		avgWidth() const;
    int 		width(const char* str) const;
    int			ascent() const; 
    int			descent() const; 

    const char*		key() const		{ return key_; }

protected: 

    // don't change order of these attributes!
    QFont*		qfont_; 
    QFontMetrics&	qfontmetrics_; 

    BufferString	key_;

    void		updateMetrics();

};


class uiFontList : public CallBacker
{
    friend class	uiSetFonts;

public:

    static int		nrKeys();
    static const char*	key(int);
    static void		listKeys(BufferStringSet&);

    static uiFont&	get(const char* key=0);
    static uiFont&	getFromQfnt(QFont*);

    static uiFont&	add(const char* key,const FontData&);
    static uiFont&	add(const char* key,
			    const char* f=FontData::defaultFamily(),
			    int ptsz=FontData::defaultPointSize(),
			    FontData::Weight w=FontData::defaultWeight(),
			    bool it=FontData::defaultItalic());

    static void		use(const Settings&);
    static void		update(Settings&);

protected:

    static ObjectSet<uiFont> fonts;
    static void		initialise();
    static uiFont&	gtFont(const char* key,const FontData* =0,
			       const QFont* =0 );

private:

    static bool		inited;

    static void		addOldGuess(const Settings&,const char*,int);
    static void		removeOldEntries(Settings&);

};


#endif
