#ifndef picksetio_h
#define picksetio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "autosaver.h"
#include "pickset.h"
class Executor;


namespace Pick
{
class SetMgr;


/*!\brief Loader for Pick::Set's. When done, sets should be available in
  the Pick::SetMgr of your choice.

  Note that the Executor will never fail. All error handling should be tied
  to the SetLoader.

 */

mExpClass(Geometry) SetLoader
{
public:

			SetLoader(const MultiID&);
			SetLoader(const TypeSet<MultiID>&);

    void		setSetMgr( SetMgr* m )	{ setmgr_ = m; }
    SetMgr&		getSetMgr() const;

    Executor*		getLoader() const;	//!< if need user feedback
    bool		load() const;		//!< if you can wait for it

    bool		allOK() const
			{ return available_.size() == toload_.size(); }

    const uiStringSet&	errMsgs() const		{ return errmsgs_; }
    const TypeSet<MultiID>& requested() const	{ return toload_; }
    const TypeSet<MultiID>& available() const	{ return available_; }

protected:

    TypeSet<MultiID>	toload_;
    mutable TypeSet<MultiID> available_;
    mutable uiStringSet	errmsgs_;
    mutable SetMgr*	setmgr_;
    friend class	SetLoaderExec;

};


/*!\brief Saveable for Pick::Set. */

mExpClass(Geometry) SetSaver : public OD::AutoSaveable
{
public:

			SetSaver(const Pick::Set&);

    const Pick::Set&	pickSet() const
			{ return static_cast<const Pick::Set&>( monitored() ); }

    virtual BufferString getFingerPrint() const;

protected:

    virtual bool	doStore(const IOObj&) const;

};

} // namespace Pick


#endif
