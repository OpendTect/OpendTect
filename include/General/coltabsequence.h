#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-3-2000
________________________________________________________________________

-*/


#include "generalmod.h"
#include "color.h"
#include "enums.h"
#include "namedobj.h"
#include "notify.h"
#include "geometry.h"

class BufferStringSet;


namespace ColTab
{

/*!\brief Maps from [0,1] -> Color

  Standard color sequences ('Color tables') are read at program start,
  including the 'user defined' ones. Users can overrule the standard ones.

  Sequences cannot be scaled, try the Mapper.

 */

mExpClass(General) Sequence : public NamedMonitorable
{
public:

    typedef float			PosType; //!< 0 <= x <= 1
    typedef TypeSet<float>::size_type	size_type;
    typedef size_type			IdxType;
    typedef unsigned char		ValueType;
    typedef std::pair<PosType,ValueType> TranspPtType;

    enum Type		{ System, User, Edited };

			Sequence();		//!< Empty
			Sequence(const char*);	//!< Find by name in SeqMgr
			~Sequence();
			mDeclInstanceCreatedNotifierAccess(Sequence);
			mDeclMonitorableAssignment(Sequence);

    bool		operator==(const Sequence&) const;
    bool		operator!=( const Sequence& oth ) const
			{ return !(*this == oth); }

    Color		color(PosType) const; //!< 0 <= pos <= 1

    inline bool		isSys() const		{ return type()==System; }
    mImplSimpleMonitoredGetSet(inline,type,setType,Type,type_,cTypeChange())

    inline bool		isEmpty() const		{ return size() < 1; }
    size_type		size() const;
    PosType		position(IdxType) const;
    ValueType		r(IdxType) const;
    ValueType		g(IdxType) const;
    ValueType		b(IdxType) const;

    size_type		transparencySize() const;
    TranspPtType	transparency(IdxType) const;
    ValueType		transparencyAt(PosType) const;
    void		setTransparency(TranspPtType);
    void		changeTransparency(IdxType,TranspPtType);
    void		removeTransparencies();
    void		removeTransparencyAt(IdxType);
    bool		hasTransparency() const;

    mImplSimpleMonitoredGetSet(inline,nrSegments,setNrSegments,size_type,
				    nrsegments_,cNrSegmentsChange())
			/*!<nrsegments > 0 divide the ctab in equally wide
			    nrsegments == 0 no segmentation
			    nrsegments == -1 constant color between markers.*/
    bool		isSegmentized() const		{ return nrSegments(); }

    void		changeColor(IdxType,ValueType,ValueType,ValueType);
    void		changePos(IdxType,PosType);
    IdxType		setColor(PosType, //!< Insert or change
				 ValueType,ValueType,ValueType);
    void		removeColor(IdxType);
    void		removeAllColors();
    void		flipColor();
    void		flipTransparency();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    mImplSimpleMonitoredGetSet(inline,undefColor,setUndefColor,
				Color,undefcolor_,cUdfColChange())
    mImplSimpleMonitoredGetSet(inline,markColor,setMarkColor,
				Color,markcolor_,cMarkColChange())

    static ChangeType	cTypeChange()		{ return 2; }
    static ChangeType	cColorChange()		{ return 3; }
    static ChangeType	cTransparencyChange()	{ return 4; }
    static ChangeType	cNrSegmentsChange()	{ return 5; }
    static ChangeType	cUdfColChange()		{ return 6; }
    static ChangeType	cMarkColChange()	{ return 7; }

    static const char*	sKeyValCol();
    static const char*	sKeyMarkColor();
    static const char*	sKeyUdfColor();
    static const char*	sKeyTransparency();
    static const char*	sKeyCtbl();
    static const char*	sKeyNrSegments();
    static const char*	sDefaultName(bool for_seismics=false);

protected:

    TypeSet<PosType>		x_;
    TypeSet<ValueType>		r_;
    TypeSet<ValueType>		g_;
    TypeSet<ValueType>		b_;
    TypeSet<TranspPtType>	tr_;

    Color			undefcolor_;
    Color			markcolor_;
    Type			type_;
    size_type			nrsegments_;

    inline size_type	gtSize() const	{ return x_.size(); }

    PosType		snapToSegmentCenter(PosType) const;
    ValueType		gtTransparencyAt(PosType) const;
    bool		chgColor(IdxType,ValueType,ValueType,ValueType);
    bool		rmColor(IdxType);

};


/*!\brief Manages Sequences; reads/writes system or user-defined

  Has a singleton instance ColTab::SM().

 */

mExpClass(General) SeqMgr : public CallBacker
{
public:

    typedef ObjectSet<Sequence>::size_type  size_type;
    typedef size_type			    IdxType;

    void		refresh();

    size_type		size() const		{ return seqs_.size(); }
    IdxType		indexOf(const char*) const;
    const Sequence*	get( IdxType idx ) const	{ return seqs_[idx]; }
    bool		get(const char*,Sequence&);
    const Sequence*	getByName(const char*) const;
    void		getSequenceNames(BufferStringSet&);
    const Sequence&	getDefault() const	{ return getAny(0); }
    const Sequence&	getAny(const char* key) const;
			//!< returns with key, or a nice one anyway

    void		set(const Sequence&); //!< if name not yet present, adds
    void		remove(IdxType);

    bool		write(bool sys=false,bool applsetup=true);

    Notifier<SeqMgr>	seqAdded;
    Notifier<SeqMgr>	seqRemoved;

			~SeqMgr();

protected:

			SeqMgr();


    ObjectSet<Sequence> seqs_;

    friend mGlobal(General) SeqMgr&	SM();

    void		addFromPar(const IOPar&,bool);
    void		add( Sequence* seq )
			{ seqs_ += seq; seqAdded.trigger(); }
    void		readColTabs();

private:

    ObjectSet<Sequence> removedseqs_;

};

mGlobal(General) SeqMgr& SM();

} // namespace ColTab

namespace RGBBlend
{
    mGlobal(General) ColTab::Sequence	getRedColTab();
    mGlobal(General) ColTab::Sequence	getBlueColTab();
    mGlobal(General) ColTab::Sequence	getGreenColTab();
    mGlobal(General) ColTab::Sequence	getTransparencyColTab();
    mGlobal(General) ColTab::Sequence	getColTab(int nr);
}
