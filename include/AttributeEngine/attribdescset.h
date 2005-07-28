#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.11 2005-07-28 14:28:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "multiid.h"

class BufferStringSet;
class IOPar;

namespace Attrib 
{
class Desc;

class DescSet
{
public:
    				~DescSet() { removeAll(); }
    DescSet*			clone() const;
    DescSet*			optimizeClone(int targetid) const;
    DescSet*      		optimizeClone(const TypeSet<int>&) const;
    				/*!< Only clones stuff needed to calculate
				     the attrib with the ids given */

    int				addDesc(Desc*,int newid=-1);
				/*!<\returns id of the attrib */

    Desc*       		getDesc(int id);
    const Desc*			getDesc(int id) const;

    int				nrDescs() const;
    int				getID(const Desc&) const;
    int				getID(int) const;
    int				getID(const char* ref,bool isusrref) const;
    void			getIds(TypeSet<int>&) const;
    int				getStoredID(const char* lk,int selout=0,
	    				    bool create=true);
    bool 			getFirstStored(Pol2D p2d, MultiID& key) const;
    Desc* 			getFirstStored(Pol2D p2d);

    void			removeDesc(int id);
    void			removeAll();
    int                 	removeUnused(bool removestored=false);
				//!< Removes unused hidden attributes.
				//!< Removed stored attribs if not available
				//!< or if removestored flag is true;
				//!< Returns total removed.
    bool 			isAttribUsed( int id ) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&,BufferStringSet* errmsgs=0);
    bool			createSteeringDesc( const IOPar&, BufferString,
						    ObjectSet<Desc>&, 
						    BufferStringSet* errmsgs=0);
	    

    bool			is2D() const;
    const char*			errMsg() const;
    static const char*		highestIDStr()		{ return "MaxNrKeys"; }
    static const char*		definitionStr()		{ return "Definition"; }
    static const char*		userRefStr()		{ return "UserRef"; }
    static const char*		inputPrefixStr()	{ return "Input"; }
    static const char*		hiddenStr()		{ return "Hidden"; }

protected:

    int				getFreeID() const;

    ObjectSet<Desc>		descs;
    TypeSet<int>		ids;
    BufferString		errmsg;
};

}; // namespace Attrib

#endif

