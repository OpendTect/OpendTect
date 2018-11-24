#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2016
________________________________________________________________________


-*/

#include "seiscommon.h"
#include "binid.h"
#include "manobjectset.h"
#include "od_iosfwd.h"
#include "typeset.h"
class ascbinistream;
class ascbinostream;


namespace Seis
{

/*!\brief a record of visited positions. */

mExpClass(Seis) TrackRecord
{
public:

    mUseType( Pos::IdxPair,	pos_type );
    typedef od_int64		SeqNrType;

    struct Entry
    {
	static Entry*	getFrom(ascbinistream&,bool is2d);
	virtual		~Entry()		{}

	virtual const char* fileKey() const	= 0;
	virtual bool	is2D() const		= 0;

	virtual bool	isOffs() const		{ return false; }
	virtual bool	isStart() const		{ return true; }
	inline SeqNrType seqNr() const		{ return seqnr_; }
	inline pos_type	trcNr() const		{ return trcnr_; }
	virtual pos_type lineNr() const		{ return mUdf(pos_type); }
	inline pos_type	inl() const		{ return lineNr(); }
	inline pos_type	crl() const		{ return trcNr(); }
	virtual bool	isTrcNrDir() const	{ return true; }
	inline BinID	binID() const
			{ return BinID(lineNr(),trcNr()); }

	inline void	setSeqNr( SeqNrType s )	{ seqnr_ = s; }
	inline void	setTrcNr( pos_type t )	{ trcnr_ = t; }
	virtual void	setLineNr(pos_type)	{}
	inline void	setInl( pos_type i )	{ setLineNr( i ); }
	inline void	setCrl( pos_type i )	{ setTrcNr( i ); }
	virtual void	setIsTrcNrDir( bool )	{}

	bool		dump(ascbinostream&) const;

    protected:
			Entry( SeqNrType seqnr, pos_type trcnr )
			    : seqnr_(seqnr)
			    , trcnr_(trcnr)	{}

	SeqNrType	seqnr_;
	pos_type	trcnr_;

    };

    typedef ObjectSet<Entry>	EntrySet;
    typedef EntrySet::size_type	idx_type;

			TrackRecord(Seis::GeomType);
			~TrackRecord()		{ setEmpty(); }

    inline bool		is2D() const		{ return is2d_; }
    inline bool		isPS() const		{ return isps_; }

    inline void		setEmpty()		{ deepErase(entries_); }
    inline EntrySet&	entries()		{ return entries_; }
    inline const EntrySet& entries() const	{ return entries_; }

    TrackRecord&	addStartEntry(SeqNrType,const BinID&,pos_type step,
				      bool diriscrl);
    inline TrackRecord&	addStartEntry( SeqNrType s, int trcnr, pos_type st,
					bool d )
		{ return addStartEntry(s,BinID(mUdf(pos_type),trcnr),st,d);}
    TrackRecord&	addEndEntry(SeqNrType,const BinID&);
    inline TrackRecord&	addEndEntry( SeqNrType s, int trcnr )
		{ return addEndEntry(s,BinID(mUdf(pos_type),trcnr));}
    TrackRecord&	addOffsetEntry(const BinID&,const TypeSet<float>&);
    inline TrackRecord&	addOffsetEntry( int t, const TypeSet<float>& o )
		{ return addOffsetEntry( BinID(mUdf(pos_type),t), o ); }

    bool		getFrom(od_istream&,bool binary);
    bool		dump(od_ostream&,bool binary) const;

protected:

    struct StartEntry : public Entry
    {
	pos_type	step_;

    protected:
			StartEntry( SeqNrType seqnr, pos_type trcnr,
				    pos_type step )
			    : Entry(seqnr,trcnr)
			    , step_(step)		{}

    };

    struct StartEntry2D : public StartEntry
    {
			StartEntry2D( SeqNrType seqnr, pos_type tnr,
				      pos_type step )
			    : StartEntry(seqnr,tnr,step) {}

	virtual const char* fileKey() const		{ return "T"; }
	virtual bool	is2D() const			{ return true; }

    };
    struct StartEntry3D : public StartEntry
    {
			StartEntry3D( SeqNrType seqnr, pos_type iln,
				      pos_type xln, pos_type step, bool crldir )
			: StartEntry(seqnr,xln,step)
			, inl_(iln)
			, iscrldir_(crldir)		{}
	virtual const char* fileKey() const
			{ return iscrldir_ ? "T" : "L"; }

	virtual bool	is2D() const			{ return false; }
	virtual pos_type lineNr() const			{ return inl_; }
	virtual bool	isTrcNrDir() const		{ return iscrldir_; }
	virtual void	setLineNr( pos_type i )		{ inl_ = i; }
	virtual void	setIsTrcNrDir( bool yn )	{ iscrldir_ = yn; }

	pos_type	inl_;
	bool		iscrldir_;
    };
    struct StopEntry : public Entry
    {
    protected:
			StopEntry( SeqNrType seqnr, pos_type trcnr )
			    : Entry(seqnr,trcnr)	{}
	virtual const char* fileKey() const		{ return "E"; }
	virtual bool	isStart() const			{ return false;}
    };
    struct StopEntry2D : public StopEntry
    {
			StopEntry2D( SeqNrType seqnr, pos_type trcnr )
			    : StopEntry(seqnr,trcnr)	{}
	virtual bool	is2D() const			{ return true; }
    };
    struct StopEntry3D : public StopEntry
    {
			StopEntry3D( SeqNrType seqnr, pos_type iln,
				     pos_type xln )
			    : StopEntry(seqnr,xln)
			    , inl_(iln)			{}
	virtual bool	is2D() const			{ return false; }
	virtual pos_type lineNr() const			{ return inl_; }
	virtual void	setLineNr( pos_type i )		{ inl_ = i; }
	pos_type	inl_;
    };
    struct OffsEntry : public Entry
    {
	TypeSet<float>	offsets_;
	virtual const char* fileKey() const		{ return "O"; }
	virtual bool	isOffs() const			{ return true; }
    protected:
			OffsEntry( pos_type trcnr )
			    : Entry(0,trcnr)		{}
    };
    struct OffsEntry2D : public OffsEntry
    {
			OffsEntry2D( pos_type trcnr )
			    : OffsEntry(trcnr)		{}
	virtual bool	is2D() const			{ return true; }
    };
    struct OffsEntry3D : public OffsEntry
    {
			OffsEntry3D( pos_type iln, pos_type xln )
			    : OffsEntry(xln)
			    , inl_(iln)			{}
	virtual bool	is2D() const			{ return false; }
	virtual pos_type lineNr() const			{ return inl_; }
	virtual void	setLineNr( pos_type i )		{ inl_ = i; }
	pos_type		inl_;
    };

    const bool		is2d_;
    const bool		isps_;
    EntrySet		entries_;

};


mExpClass(Seis) KeyTracker
{
public:

    typedef Index_Type	pos_type;
    typedef od_int64	SeqNrType;

			KeyTracker(TrackRecord&);
    virtual		~KeyTracker()		{ finish(); }
    const TrackRecord&	trackRecord() const	{ return trackrec_; }
    void		reset();

    inline bool		is2D() const		{ return trackrec_.is2D(); }
    inline bool		isPS() const		{ return trackrec_.isPS(); }

    void		add(int trcnr,float offs=0.f);
    void		add(const BinID&,float offs=0.f);
    SeqNrType		lastSeqNr() const	{ return seqnr_; }

    od_int64		nrDone() const		{ return seqnr_; };
    void		finish();
			//!< after call, trackrecord will not be used anymore
			//!< unless you call reset()

protected:

    TrackRecord&	trackrec_;

    od_int64		seqnr_;
    BinID		prevbid_;
    bool		diriscrl_;
    pos_type		step_;
    bool		finished_;
    int			offsidx_;
    TypeSet<float>	offsets_;
    bool		offsetschanged_;

    void		addFirst(const BinID&,float);
    void		addFirstFollowUp(const BinID&,float);
    void		addNext(const BinID&,float);
    void		addNextPS(const BinID&,float);
    void		checkCurOffset(float);
    void		getNewIncs(const BinID&);
    void		getNextPredBinID(BinID&) const;
    bool		isSamePos(const BinID&,const BinID&) const;
    void		addStartEntry(SeqNrType,const BinID&);
    void		addEndEntry(SeqNrType,const BinID&);
    void		addOffsetEntry();

};


} // namespace Seis
