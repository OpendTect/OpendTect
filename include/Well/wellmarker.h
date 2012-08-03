#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.21 2012-08-03 12:42:21 cvsbruno Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "color.h"

namespace Well
{

class Track;

/*!\brief Marker, should be attached to Strat level

  Can be unattached, then uses the fallback name and color. 

*/

mClass Marker : public ::NamedObject
{
public:

			Marker( const char* nm=0, float dh=0 )
			: ::NamedObject(nm)
			, dah_(dh)
			, levelid_(-1)		{}
			Marker(int lvlid,float dh);
			Marker(const Marker&);

    float		dah() const		{ return dah_; }
    void		setDah( float v )	{ dah_ = v; }
    int			levelID() const		{ return levelid_; }
    void		setLevelID( int id )	{ levelid_ = id; }

    const BufferString&	name() const;
    Color		color() const;

    static const char*	sKeyDah();

    // setName() and setColor() only used as fallback, if not attached to level
    void		setColor( Color col )	{ color_ = col; }

protected:

    float		dah_;
    int			levelid_;
    Color		color_;
};



mClass MarkerSet : public ObjectSet<Marker>
{
public:
    			MarkerSet()		{}

    virtual ObjectSet<Marker>& operator +=(Marker*);

    const Marker* 	getByName(const char* nm) const { return gtByName(nm); }
    Marker* 		getByName(const char* nm) 	{ return gtByName(nm); }

    const Marker* 	getByLvlID(int id) const	{ return gtByLvlID(id);}
    Marker* 		getByLvlID(int id) 		{ return gtByLvlID(id);}

    bool		isPresent(const char* n) const 	{ return getByName(n); }
    int			indexOf(const char*) const;		  
    void		insertNew(Well::Marker*); //becomes mine

    int			indexOf( const Marker* m ) const
			{ return ObjectSet<Marker>::indexOf(m); }	
    bool		isPresent( const Marker* m ) const
			{ return ObjectSet<Marker>::isPresent(m); }	

    void		getNames(BufferStringSet&) const;

protected:

    Marker* 		gtByName(const char*) const;
    Marker* 		gtByLvlID(int) const;
};


} // namespace Well

#endif
