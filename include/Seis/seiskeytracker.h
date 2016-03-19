#ifndef seiskeytracker_h
#define seiskeytracker_h

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
class ascbinistream;
class ascbinostream;


namespace Seis
{

/*!\brief a record of visited positions. */

mExpClass(Seis) TrackRecord
{
public:

    typedef Index_Type			IdxType;

    struct Entry
    {
	enum Type	{ TStart, LStart, Stop, OffsChg };
	static const char* fileKey(Type);
	static Entry*	getFrom(ascbinistream&,bool is2d);

	Type		type_;
	IdxType		linenr_;
	IdxType		trcnr_;

	inline bool	isOffs() const		{ return type_ == OffsChg; }
	inline bool	isStart() const		{ return type_ < Stop; }
	inline bool	isTrcNrDir() const	{ return type_ != LStart; }
	inline const char* fileKey() const	{ return fileKey( type_ ); }
	inline IdxType	trcNr() const		{ return trcnr_; }
	inline IdxType	lineNr() const		{ return linenr_; }
	inline IdxType	crl() const		{ return trcnr_; }
	inline IdxType	inl() const		{ return linenr_; }
	inline BinID	binID() const		{ return BinID(linenr_,trcnr_);}

	bool		dump(ascbinostream&,bool is2d) const;

    protected:
			Entry( Type typ, IdxType trcnr )
			    : type_(typ)
			    , trcnr_(trcnr)
			    , linenr_(mUdf(int))    {}

    };

    typedef ManagedObjectSet<Entry>	EntrySet;
    typedef EntrySet::size_type		ArrIdxType;

			TrackRecord(Seis::GeomType);

    inline bool		is2D() const		{ return is2d_; }
    inline bool		isPS() const		{ return isps_; }

    inline void		setEmpty()		{ entries_.erase(); }
    inline EntrySet&	entries()		{ return entries_; }
    inline const EntrySet& entries() const	{ return entries_; }

    TrackRecord&	addStartEntry(const BinID&,IdxType step,bool diriscrl);
    inline TrackRecord&	addStartEntry( int trcnr, IdxType s, bool d )
			{ return addStartEntry(BinID(mUdf(IdxType),trcnr),s,d);}
    TrackRecord&	addEndEntry(const BinID&);
    inline TrackRecord&	addEndEntry( int trcnr )
			{ return addEndEntry(BinID(mUdf(IdxType),trcnr));}
    TrackRecord&	addOffsetEntry(const BinID&,const TypeSet<float>&);
    inline TrackRecord&	addOffsetEntry( int t, const TypeSet<float>& o )
			{ return addOffsetEntry( BinID(mUdf(IdxType),t), o ); }

    bool		getFrom(od_istream&,bool binary);
    bool		dump(od_ostream&,bool binary) const;

protected:

    struct StartEntry : public Entry
    {
	IdxType		step_;
    protected:
			StartEntry( Type typ, IdxType trcnr, IdxType step )
			    : Entry(typ,trcnr), step_(step)	{}
    };
    struct StartEntry2D : public StartEntry
    {
			StartEntry2D( IdxType crl, IdxType step )
			    : StartEntry(TStart,crl,step)	{}
    };
    struct StartEntry3D : public StartEntry
    {
		    StartEntry3D( IdxType inl, IdxType crl, IdxType step,
				   bool crldir )
			: StartEntry(crldir?TStart:LStart,crl,step)
							    { linenr_ = inl; }
    };
    struct StopEntry : public Entry
    {
    protected:
			StopEntry( IdxType trcnr )
			    : Entry(Stop,trcnr)			{}
    };
    struct StopEntry2D : public StopEntry
    {
			StopEntry2D( IdxType trcnr )
			    : StopEntry(trcnr)			{}
    };
    struct StopEntry3D : public StopEntry
    {
			StopEntry3D( IdxType inl, IdxType crl )
			    : StopEntry(crl)		    { linenr_ = inl; }
    };
    struct OffsEntry : public Entry
    {
	TypeSet<float>	offsets_;
    protected:
			OffsEntry( IdxType trcnr )
			    : Entry(OffsChg,trcnr)		{}
    };
    struct OffsEntry2D : public OffsEntry
    {
			OffsEntry2D( IdxType trcnr )
			    : OffsEntry(trcnr)			{}
    };
    struct OffsEntry3D : public OffsEntry
    {
			OffsEntry3D( IdxType inl, IdxType crl )
			    : OffsEntry(crl)		    { linenr_ = inl; }
    };

    const bool		is2d_;
    const bool		isps_;
    EntrySet		entries_;

};


/*!\brief builds TrackRecord using provided positions; finds changes that
	    cannot be predicted from previous changes. */

mExpClass(Seis) KeyTracker
{
public:

    typedef Index_Type	IdxType;

			KeyTracker(TrackRecord&);
    virtual		~KeyTracker()		{ finish(); }
    const TrackRecord&	trackRecord() const	{ return trackrec_; }
    void		reset();

    inline bool		is2D() const		{ return trackrec_.is2D(); }
    inline bool		isPS() const		{ return trackrec_.isPS(); }

    void		add(int trcnr,float offs=0.f);
    void		add(const BinID&,float offs=0.f);

    od_int64		nrDone() const	    { return nrhandled_; };
    void		finish();
			//!< after call, trackrecord will not be used anymore
			//!< unless you call reset()

protected:

    TrackRecord&	trackrec_;

    od_int64		nrhandled_;
    BinID		prevbid_;
    bool		diriscrl_;
    IdxType		step_;
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
    void		addStartEntry(const BinID&);
    void		addEndEntry(const BinID&);
    void		addOffsetEntry();

};


} // namespace Seis

#endif
