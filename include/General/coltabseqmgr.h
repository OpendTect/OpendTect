#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		23-3-2000
________________________________________________________________________

-*/


#include "coltabsequence.h"
#include "ptrman.h"


namespace ColTab
{


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
    typedef size_type				idx_type;
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
    idx_type		indexOf(const char*) const;
    idx_type		indexOf(const Sequence&) const;
    ConstRef		getByIdx(idx_type) const;
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
    idx_type		idxOf(const char*) const;
    const Sequence*	gtAny(const char*,bool) const;
    void		doAdd(Sequence*,bool issys);
    void		addFromPar(const IOPar&,bool);
    void		rollbackFrom(const SequenceManager&);

    static idx_type	gtIdxOf(const ObjectSet<Sequence>&,const char*);

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
    inline idx_type	indexOf( const OD::String& s ) const
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
