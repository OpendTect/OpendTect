#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "color.h"
#include "enums.h"
#include "namedobj.h"
#include "geometry.h"

class BufferStringSet;


namespace ColTab
{

/*!\brief Maps from [0,1] -> Color

  Standard color sequences ('Color tables') are read at program start,
  including the 'user defined' ones. Users can overrule the standard ones.

  Sequences cannot be scaled, try the Mapper.

 */

mExpClass(General) Sequence : public NamedCallBacker
{
public:
			Sequence();
			Sequence(const char*);	//!< Find by name in SeqMgr
			Sequence(const Sequence&);
			~Sequence();

    enum Type		{ System, User, Edited };

    Sequence&		operator=(const Sequence&);
    bool		operator==(const Sequence&) const;
    bool		operator!=(const Sequence&) const;

    bool		isSys() const		{ return type_==System; }
    Type		type() const		{ return type_; }
    void		setType( Type tp )	{ type_ = tp; }

    OD::Color		color(float pos) const; //!< 0 <= pos <= 1

    inline bool		validIdx(int idx) const	{ return x_.validIdx(idx); }
    bool		canChangePosition(int idx) const;
    inline bool		isEmpty() const		{ return x_.isEmpty(); }
    inline int		size() const		{ return x_.size(); }
    inline float	position( int idx ) const { return x_[idx]; }
    inline unsigned char r( int idx ) const	{ return r_[idx]; }
    inline unsigned char g( int idx ) const	{ return g_[idx]; }
    inline unsigned char b( int idx ) const	{ return b_[idx]; }

    int			transparencySize() const	{ return tr_.size(); }
    Geom::Point2D<float> transparency( int idx ) const	{ return tr_[idx]; }
    float		transparencyAt(float) const;
    void		setTransparency(Geom::Point2D<float>);
			/*!<x goes from 0 to 1, trans from 0 to 255 */
    void		changeTransparency(int,Geom::Point2D<float>);
    void		removeTransparencies();
    void		removeTransparencyAt(int);
    bool		hasTransparency() const;

    int			nrSegments() const		{ return nrsegments_; }
    void		setNrSegments(int n)		{ nrsegments_ = n; }
			/*!<nrsegments > 0 divide the ctab in equally wide
			    nrsegments == 0 no segmentation
			    nrsegments == -1 constant color between markers.*/


    bool		isSegmentized() const		{ return nrsegments_; }

    void		changeColor(int idx,const OD::Color&);
    void		changeColor(int,
				    unsigned char,unsigned char,unsigned char);
    void		changePos(int,float);
    int			setColor(float pos,const OD::Color&);
    int			setColor(float pos,
				 unsigned char,unsigned char,unsigned char);
			//!<Inserts or changes color
    void		removeColor(int);
    void		removeAllColors();
    void		flipColor();
    void		flipTransparency();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Notifier<Sequence>	colorChanged;
    Notifier<Sequence>	transparencyChanged;
    Notifier<Sequence>	toBeRemoved;

    const OD::Color&	undefColor() const
			{ return undefcolor_; }
    void		setUndefColor( OD::Color c )
			{ undefcolor_ = c; triggerAll(); }
    const OD::Color&	markColor() const
			{ return markcolor_; }
    void		setMarkColor( OD::Color c )
			{ markcolor_ = c; triggerAll(); }

    void		simplify();

    static const char*	sKeyValCol();
    static const char*	sKeyMarkColor();
    static const char*	sKeyUdfColor();
    static const char*	sKeyTransparency();
    static const char*	sKeyCtbl();
    static const char*	sKeyNrSegments();
    static const char*  sKeyRainbow();

protected:

    TypeSet<float>		x_;
    TypeSet<unsigned char>	r_;
    TypeSet<unsigned char>	g_;
    TypeSet<unsigned char>	b_;
    TypeSet< Geom::Point2D<float> > tr_;

    OD::Color		undefcolor_		= OD::Color::LightGrey();
    OD::Color		markcolor_		= OD::Color::DgbColor();
    Type		type_;
    int			nrsegments_			= 0;

    inline void		triggerAll() {	colorChanged.trigger();
					transparencyChanged.trigger(); }
    float		snapToSegmentCenter(float) const;
};


/*!\brief Manages Sequences; reads/writes system or user-defined

  Has a singleton instance ColTab::SM().

 */

mExpClass(General) SeqMgr : public CallBacker
{
public:

    void		refresh();

    int			size() const		{ return seqs_.size(); }
    int			indexOf(const char*) const;
    const Sequence*	get( int idx ) const	{ return seqs_[idx]; }
    bool		get(const char*,Sequence&);
    void		getSequenceNames(BufferStringSet&);
    const Sequence*	getAny(const char* key) const;
			//!< returns with key, or a nice one anyway

    void		set(const Sequence&); //!< if name not yet present, adds
    void		remove(int);
    bool		rename(const char* oldnm,const char* newnm);

    bool		write(bool sys=false,bool applsetup=true);

    Notifier<SeqMgr>	seqAdded;
    Notifier<SeqMgr>	seqRemoved;

			~SeqMgr();

protected:

			SeqMgr();

    ObjectSet<Sequence>	seqs_;

    friend mGlobal(General) SeqMgr&	SM();

    void		addFromPar(const IOPar&,bool);
    void		add( Sequence* seq )
			{ seqs_ += seq; seqAdded.trigger(); }
    void		readColTabs();
};

mGlobal(General) SeqMgr& SM();

} // namespace ColTab
