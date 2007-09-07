#ifndef colortab_h
#define colortab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-3-2000
 RCS:		$Id: coltabsequence.h,v 1.1 2007-09-07 11:21:01 cvsbert Exp $
________________________________________________________________________

-*/


#include "color.h"
#include "enums.h"
#include "namedobj.h"
#include "geometry.h"

class IOPar;
class BufferStringSet;
class NamedBufferStringSet;

namespace ColTab
{

/*!\brief Maps from [0,1] -> Color

  Standard color sequences ('Color tables') are read at program start,
  including the 'user defined' ones. Users can overrule the standard ones.

  Sequences cannot be scaled, try the Mapper.
 
 */

class Sequence : public NamedObject
{
public:

			Sequence();
			Sequence(const char*);	//!< Find by name
			Sequence(const Sequence&);
    Sequence&		operator=(const Sequence&);

    Color		color(float pos) const; //!< 0 <= pos <= 1

    inline int		size() const		{ return x_.size(); }
    inline float	position( int idx ) const { return x_[idx]; }
    inline unsigned char r( int idx ) const	{ return r_[idx]; }
    inline unsigned char g( int idx ) const	{ return g_[idx]; }
    inline unsigned char b( int idx ) const	{ return b_[idx]; }

    int			transparencySize() const	{ return tr_.size(); }
    Geom::Point2D<float> transparency( int idx ) const	{ return tr_[idx]; }
    float		transparencyAt(float) const;
    bool		hasTransparency() const;

    void		changeColor(int,
	    			    unsigned char,unsigned char,unsigned char);
    void		changePos(int,float);
    void		setColor(float pos, //!< Insert or change
	    			 unsigned char,unsigned char,unsigned char);
    void		setTransparency(Geom::Point2D<float>);
    void		removeTransparencies()		{ tr_.erase(); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    enum Src		{ UsrDef, Both, Sys };
    static void		getNames(NamedBufferStringSet&,Src c=Both);
    static bool		get(const char*,Sequence&,Src c=Both);
    static void		add(const Sequence&);

    static const char*	sKeyValCol;
    static const char*	sKeyMarkColor;
    static const char*	sKeyUdfColor;
    static const char*	sKeyTransparency;
    static const char*	sKeyCtbl;

    static void		getStdTabPars(ObjectSet<IOPar>&);
    			//!< Intended for internal and sysadm use
    static bool		putStdTabPars(const ObjectSet<IOPar>&);
    			//!< Intended for internal and sysadm use

protected:

    TypeSet<unsigned char>	r_;
    TypeSet<unsigned char>	g_;
    TypeSet<unsigned char>	b_;
    TypeSet<float>		x_;
    TypeSet< Geom::Point2D<float> > tr_;

    Color		undefcolor_;
    Color		markcolor_;

    static void		add(const IOPar&,BufferStringSet*,ObjectSet<IOPar>*);
    			//!< Adds table names and/or Sequence IOPar instances
    			//!< from an IOPar with multiple color table entries
};

} // namespace ColTab


#endif
