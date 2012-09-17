#ifndef fontdata_h
#define fontdata_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/07/2000

 RCS:           $Id: fontdata.h,v 1.15 2011/02/04 17:49:38 cvskris Exp $

______________________________________________________________________

-*/

#include "enums.h"
#include "bufstring.h"

//#ifdef BASIC_EXPORTS
//# define mBasic __declspec(dllexport)
//#else
//# define mBasic __declspec(dllimport)
//#endif

//class mBasic FontData
mClass FontData
{			//!< Data needed to make an actual font
public:    

    enum Weight		{ Light, Normal, DemiBold, Bold, Black };
			DeclareEnumUtils(Weight)
    static int		numWeight(Weight);
    static Weight	enumWeight(int);

			FontData( int ptsz=defaultPointSize(),
				  const char* fam=defaultFamily(),
				  Weight wght=defaultWeight(),
				  bool ital=defaultItalic() )
                        : family_(fam)
			, pointsize_(ptsz)
			, weight_(wght)
			, italic_(ital)		{}

			FontData( const char* s )
			: family_(defaultFamily())
			, pointsize_(defaultPointSize())
			, weight_(defaultWeight())
			, italic_(defaultItalic())
			{ getFrom(s); }

			//! Store/retrieve (in FileMultiString format).
    void		getFrom(const char*);
    void		putTo(BufferString&) const;

    const char*		family() const		{ return family_; }
    int			pointSize() const	{ return pointsize_; }
    Weight		weight() const		{ return weight_; }
    bool		isItalic() const	{ return italic_; }

    void		setFamily( const char* f ) { family_ = f; }
    void		setPointSize( int ps )	{ pointsize_ = ps; }
    void		setWeight( Weight w )	{ weight_= w; }
    void		setItalic( bool yn )	{ italic_ = yn; }

    static const char*	defaultFamily();
    static int		defaultPointSize();
    static Weight	defaultWeight();
    static bool		defaultItalic();

    static void		setDefaultFamily( const char* f );
    static void		setDefaultPointSize( int ps );
    static void		setDefaultWeight( Weight w );
    static void		setDefaultItalic( bool yn );

    static const char* const* universalFamilies();
			//!< array of strings with null as last element

    static const char* const* defaultKeys();
			//!< array of strings with null as last element

    enum StdSz		{ Control=0, 
			  GraphicsMed, GraphicsSmall, GraphicsLarge,
			  ControlSmall, ControlLarge, Fixed };
    static const char* key( StdSz ss );
 
protected:
 
    BufferString	family_;
    int			pointsize_;
    Weight		weight_;
    bool		italic_;

 
};


#endif
