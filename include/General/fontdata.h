#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "enums.h"
#include "bufstring.h"

/*!\brief Data needed to make an actual font. */

mExpClass(General) FontData
{
public:

    enum Weight		{ Light, Normal, DemiBold, Bold, Black };
			mDeclareEnumUtils(Weight)
    static int		numWeight(Weight);
    static Weight	enumWeight(int);

			FontData(int ptsz=defaultPointSize(),
				 const char* fam=defaultFamily(),
				 Weight wght=defaultWeight(),
				 bool ital=defaultItalic());
			FontData(const char* fms); //! Calls getFrom

    bool		operator ==(const FontData&) const;
    bool		operator !=(const FontData&) const;
    FontData&		operator =(const FontData&);

			//! Store/retrieve (in FileMultiString format).
    bool		getFrom(const char* fms);
    void		putTo(BufferString&) const;

    const char*		family() const		{ return family_; }
    int			pointSize() const	{ return pointsize_; }
    Weight		weight() const		{ return weight_; }
    bool		isItalic() const	{ return italic_; }

    void		setFamily( const char* f ) { family_ = f; }
    void		setPointSize( int ps )	{ pointsize_ = ps; }
    void		setWeight( Weight w )	{ weight_= w; }
    void		setItalic( bool yn )	{ italic_ = yn; }

    void		setStyleName(const char*);
    const char*		styleName() const;

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

    enum StdSz		{ Control=0, Graphics2D, Graphics3D, Fixed,
			  Graphics2DSmall, Graphics2DLarge,
			  ControlSmall, ControlLarge };
    static const char* key( StdSz ss );

protected:

    BufferString	family_		= defaultFamily();
    int			pointsize_	= defaultPointSize();
    Weight		weight_		= defaultWeight();
    bool		italic_		= defaultItalic();
    BufferString	stylename_	= "Regular";
};
