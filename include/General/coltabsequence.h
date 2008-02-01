#ifndef coltabsequence_h
#define coltabsequence_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-3-2000
 RCS:		$Id: coltabsequence.h,v 1.3 2008-02-01 07:59:45 cvsnanne Exp $
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
			Sequence(const char*);	//!< Find by name in SeqMgr
			Sequence(const Sequence&);
			~Sequence();
    Sequence&		operator=(const Sequence&);
    bool		isSys() const		{ return issys_; }

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
    void		removeTransparencies();

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    Notifier<Sequence>	colorChanged;
    Notifier<Sequence>	transparencyChanged;
    Notifier<Sequence>	toBeRemoved;

    Color		undefColor() const
			{ return undefcolor_; }
    void		setUndefColor( Color c )
			{ undefcolor_ = c; triggerAll(); }
    Color		markColor() const
			{ return markcolor_; }
    void		setMarkColor( Color c )
			{ markcolor_ = c; triggerAll(); }
    void		setIsSys( bool yn )	{ issys_ = yn; }

    static const char*	sKeyValCol;
    static const char*	sKeyMarkColor;
    static const char*	sKeyUdfColor;
    static const char*	sKeyTransparency;
    static const char*	sKeyCtbl;

protected:

    bool			issys_;

    TypeSet<unsigned char>	r_;
    TypeSet<unsigned char>	g_;
    TypeSet<unsigned char>	b_;
    TypeSet<float>		x_;
    TypeSet< Geom::Point2D<float> > tr_;

    Color		undefcolor_;
    Color		markcolor_;

    inline void		triggerAll() {	colorChanged.trigger();
					transparencyChanged.trigger(); }
};


/*!\brief Manages Sequences; reads/writes system or user-defined

  Has a singleton instance ColTab::SM().
 
 */

class SeqMgr : public CallBacker
{
public:

    int			size() const		{ return seqs_.size(); }
    int			indexOf(const char*) const;
    const Sequence*	get( int idx ) const	{ return seqs_[idx]; }
    bool		get(const char*,Sequence&);

    void		set(const Sequence&); //!< if name not yet present, adds
    void		remove(int);

    bool		write(bool sys=false,bool applsetup=true);


    Notifier<SeqMgr>	seqAdded;
    Notifier<SeqMgr>	seqRemoved;

protected:

    			SeqMgr();
			~SeqMgr()	{}

    ObjectSet<Sequence>	seqs_;

    friend SeqMgr&	SM();

    void		addFromPar(const IOPar&);
    void		add( Sequence* seq )
    			{ seqs_ += seq; seqAdded.trigger(); }
};

SeqMgr& SM();


} // namespace ColTab


#endif
