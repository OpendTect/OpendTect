#ifndef wellman_h
#define wellman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "sets.h"
#include "bufstring.h"
#include <bitset>

class IOObj;
class MultiID;
class BufferStringSet;

namespace Well
{

class Data;

/*\brief Tells the Well Manager what you need to be loaded.*/

#define mWellNrSubObjTypes 9

enum SubObjType		{ Inf=0, Trck=1, D2T=2, CSMdl=3, Mrkrs=4, Logs=5,
			   LogInfos=6, DispProps2D=7, DispProps3D=8 };


mExpClass(Well) LoadReqs
{
public:

			LoadReqs(bool addall=true);
			LoadReqs(SubObjType);
			LoadReqs(SubObjType,SubObjType);
			LoadReqs(SubObjType,SubObjType,SubObjType);
    static LoadReqs	All();
    bool		operator ==( const LoadReqs& oth ) const
						{ return reqs_ == oth.reqs_; }

    LoadReqs&		add(SubObjType);
    LoadReqs&		remove( SubObjType typ ) { reqs_[typ]=0; return *this; }
    void		setToAll()		{ *this = All(); }
    void		setEmpty()		{ reqs_.reset(); }
    void		include(const LoadReqs&);

    bool		includes( SubObjType typ ) const
						{ return reqs_[typ]; }

protected:

    std::bitset<mWellNrSubObjTypes>		reqs_;
};



/*!
\brief Well manager
*/

mExpClass(Well) Man
{
public:
			~Man();

    void		removeObject( const Well::Data* );
    Data*		get(const MultiID&, LoadReqs reqs=LoadReqs());
    void		add(const MultiID&,Data*);
			//!< Data becomes mine
    Data*		release(const MultiID&);
			//!< Data becomes yours
    bool		isLoaded(const MultiID&) const;
    bool		reload(const MultiID&);

    const char*		errMsg() const		{ return msg_; }
    ObjectSet<Data>&	wells()			{ return wells_; }

    static bool		getLogNames(const MultiID&,BufferStringSet&,
				    bool forceLoad=false);
    static bool		getMarkerNames(BufferStringSet&);

protected:

			Man()				{}
    static Man*		mgr_;
    mGlobal(Well) friend Man&	MGR();

    ObjectSet<Data>	wells_;
    BufferString	msg_;

    int			gtByKey(const MultiID&) const;
};

mGlobal(Well) Man& MGR();

mGlobal(Well) IOObj* findIOObj(const char* wellnm,const char* uwi);

} // namespace Well

#endif
