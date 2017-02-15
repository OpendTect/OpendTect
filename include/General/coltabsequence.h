#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-3-2000
________________________________________________________________________

-*/


#include "coltab.h"
#include "ptrman.h"
#include "uistring.h"
#include "enums.h"
#include "sharedobject.h"


namespace ColTab
{

class SequenceManager;


/*!\brief A series of color control points able to give an (interpolated) color
	  for every position [0,1].

  Standard color sequences ('Color tables') are read at program start,
  including the 'user defined' ones. Users can overrule or 'remove' the
  standard ones. Disabled sequecens will not be shown in selectors.

  Sequences cannot be scaled, try the Mapper.

 */

mExpClass(General) Sequence : public SharedObject
{
public:

    typedef float			PosType; //!< 0 <= x <= 1
    typedef TypeSet<float>::size_type	size_type;
    typedef size_type			IdxType;
    typedef unsigned char		ValueType;
    typedef std::pair<PosType,ValueType> TranspPtType;

    enum Status		{ System, Edited, Added };
			mDeclareEnumUtils(Status);

			Sequence();		//!< Empty
			Sequence(const char*);	//!< Find by name in SeqMGR
			mDeclInstanceCreatedNotifierAccess(Sequence);
			mDeclMonitorableAssignment(Sequence);

    Color		color(PosType) const; //!< 0 <= pos <= 1

    Status		status() const;
    inline bool		isSys() const		{ return status()==System; }
    mImplSimpleMonitoredGetSet(inline,disabled,setDisabled,bool,disabled_,
						    cStatusChange())
    uiString		statusDispStr() const;

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
				    nrsegments_,cSegmentationChange())
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

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    mImplSimpleMonitoredGetSet(inline,undefColor,setUndefColor,
				Color,undefcolor_,cUdfColChange())
    mImplSimpleMonitoredGetSet(inline,markColor,setMarkColor,
				Color,markcolor_,cMarkColChange())

    static ChangeType	cStatusChange()		{ return 3; }
    static ChangeType	cColorChange()		{ return 4; }
    static ChangeType	cTransparencyChange()	{ return 5; }
    static ChangeType	cSegmentationChange()	{ return 6; }
    static ChangeType	cMarkColChange()	{ return 7; }
    static ChangeType	cUdfColChange()		{ return 8; }

    static const char*	sKeyValCol();
    static const char*	sKeyMarkColor();
    static const char*	sKeyUdfColor();
    static const char*	sKeyTransparency();
    static const char*	sKeyCtbl();
    static const char*	sKeyNrSegments();
    static const char*	sKeyDisabled();
    static const char*	sDefaultName(bool for_seismics=false);

protected:

			~Sequence();

    TypeSet<PosType>	x_;
    TypeSet<ValueType>	r_;
    TypeSet<ValueType>	g_;
    TypeSet<ValueType>	b_;
    TypeSet<TranspPtType> tr_;
    size_type		nrsegments_;

    Color		undefcolor_;
    Color		markcolor_;
    bool		disabled_;

    inline size_type	gtSize() const	{ return x_.size(); }

    PosType		snapToSegmentCenter(PosType) const;
    ValueType		gtTransparencyAt(PosType) const;
    bool		chgColor(IdxType,ValueType,ValueType,ValueType);
    bool		rmColor(IdxType);
    void		emitStatusChg() const;

    friend class	SequenceManager;

};


/*!\brief Manages Sequences; reads/writes system or user-defined.

  This class is not fully MT-safe; it does not (safely) support concurrent
  editing from different threads. That out of the way, we can do without IDs.
  The name of a sequence is its key. Names are compared case-insensitive.
  The name of a sequence can change though, watch the Sequence's notifications.

  The class has a singleton instance ColTab::SeqMGR().

 */

mExpClass(General) SequenceManager : public Monitorable
{ mODTextTranslationClass(ColTab::SequenceManager);
public:

    typedef ObjectSet<Sequence>::size_type	size_type;
    typedef size_type				IdxType;
    typedef ConstRefMan<Sequence>		ConstRef;

			mDeclMonitorableAssignment(SequenceManager);

    bool		isPresent(const char*) const;
    ConstRef		getByName(const char*) const;	//!< can be null
    ConstRef		getAny(const char*,bool seismics_if_default=true) const;
				//!< guaranteed non-null
    ConstRef		getDefault(bool for_seismics=true) const;
				//!< guaranteed non-null
    ConstRef		getFromPar(const IOPar&,const char* subky=0) const;
				//!< guaranteed non-null

			// Use monitorLock when using indexes
    size_type		size() const;
    Sequence::Status	statusOf(const Sequence&) const;
    IdxType		indexOf(const char*) const;
    IdxType		indexOf(const Sequence&) const;
    ConstRef		getByIdx(IdxType) const;
    void		getSequenceNames(BufferStringSet&) const;

    bool		needsSave() const;
    uiRetVal		write(bool sys=false,bool applsetup=true) const;

			// ID is the index ... see class remarks.
    static ChangeType	cSeqAdd()		{ return 2; }
    static ChangeType	cSeqRemove()		{ return 3; }
    static const char*	sKeyRemoved();

    mutable CNotifier<SequenceManager,ChangeData> nameChange;
			//!< provides name change notif for any of the seqs

    static const Sequence& getRGBBlendColSeq(int);
					//!< 0=red, 1=green, 2=blue 3=transp

protected:

    ObjectSet<Sequence> seqs_;
    ObjectSet<Sequence> sysseqs_;
    const bool		iscopy_;
    mutable DirtyCounter lastsaveddirtycount_;

			SequenceManager();
			~SequenceManager();

			// Not locked:
    size_type		gtSize() const;
    IdxType		idxOf(const char*) const;
    const Sequence*	gtAny(const char*,bool) const;
    void		doAdd(Sequence*,bool issys);
    void		addFromPar(const IOPar&,bool);
    void		rollbackFrom(const SequenceManager&);

    static IdxType	gtIdxOf(const ObjectSet<Sequence>&,const char*);

    void		seqChgCB(CallBacker*);

    friend mGlobal(General) const SequenceManager& SeqMGR();
    friend class	Sequence;
    // friend class	uiColSeqMan;

public:

			// leave this to the color table manager
    RefMan<Sequence>	get4Edit(const char*) const;
    void		add(Sequence*);
    void		removeByName(const char*);
    ConstRef		getSystemSeq(const char*) const;
    void		rollbackFromCopy(const SequenceManager&);
    static void		deleteInst(SequenceManager*);

    inline bool		isPresent( const OD::String& s ) const
			{ return isPresent(s.str()); }
    inline ConstRef	getByName( const OD::String& s ) const
			{ return getByName(s.str()); }
    inline ConstRef	getAny( const OD::String& s, bool fs=true ) const
			{ return getAny(s.str(),fs); }
    inline IdxType	indexOf( const OD::String& s ) const
			{ return indexOf(s.str()); }
    inline void		removeByName( const OD::String& s )
			{ removeByName(s.str()); }

};

mGlobal(General) const SequenceManager& SeqMGR();
mGlobal(General) inline SequenceManager& SeqMGR4Edit()
{ return const_cast<SequenceManager&>(SeqMGR()); }

mDeprecated mGlobal(General) inline SequenceManager& SM()
{ return SeqMGR4Edit(); }
mDeprecated typedef SequenceManager SeqMgr;


} // namespace ColTab
