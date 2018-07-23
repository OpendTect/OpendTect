#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "saveable.h"
#include "pickset.h"
#include "ptrman.h"
class Executor;


namespace Pick
{

/*!\brief Loader for Pick::Set's. When done, sets should be available in
  the Pick::SetMGR().

  Note that the Executor will never fail. All error handling should be tied
  to the SetLoader object.

 */

mExpClass(Geometry) SetLoader
{
public:

			SetLoader(const DBKey&);
			SetLoader(const DBKeySet&);

    void		setCategory( const char* cat )	{ category_ = cat; }

    Executor*		getLoader() const;	//!< if need user feedback
    bool		load() const;		//!< if you can wait for it

    bool		allOK() const
			{ return available_.size() == toload_.size(); }

    const uiRetVal&	result() const	{ return uirv_; }
    const DBKeySet& requested() const	{ return toload_; }
    const DBKeySet& available() const	{ return available_; }

    static Pick::Set*	getSingleSet(const IOObj&,uiRetVal&,const char* cat=0);

protected:

    DBKeySet		toload_;
    BufferString	category_;
    mutable DBKeySet	available_;
    mutable uiRetVal	uirv_;
    friend class	SetLoaderExec;

};


/*!\brief Saveable for Pick::Set. */

mExpClass(Geometry) SetSaver : public Saveable
{
public:

			SetSaver(const Pick::Set&);
			mDeclMonitorableAssignment(SetSaver);
			~SetSaver();

    ConstRefMan<Set>	pickSet() const;
    void		setPickSet(const Set&);

    mDeclInstanceCreatedNotifierAccess(SetSaver);

protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;

};

} // namespace Pick
