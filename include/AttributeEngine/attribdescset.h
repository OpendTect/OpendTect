#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.17 2005-10-25 12:20:57 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "multiid.h"
#include "attribdescid.h"

class BufferStringSet;
class IOPar;

namespace Attrib 
{
class Desc;

class DescSet
{
public:
    				DescSet()
				: firsttime(true) {};
				
    				~DescSet() { removeAll(); }
    DescSet*			clone() const;
    DescSet*			optimizeClone(const DescID& targetid) const;
    DescSet*      		optimizeClone(const TypeSet<DescID>&) const;
    				/*!< Only clones stuff needed to calculate
				     the attrib with the ids given */
    void			updateInputs();
    				/*!< Updates inputs for all descs in descset.
				     Necessary after cloning */

    DescID			addDesc(Desc*,DescID newid=DescID(-1,true));
				/*!< returns id of the attrib */

    Desc*       		getDesc(const DescID&);
    const Desc*			getDesc(const DescID&) const;

    int				nrDescs() const;
    DescID			getID(const Desc&) const;
    DescID			getID(int) const;
    DescID			getID(const char* ref,bool isusrref) const;
    void			getIds(TypeSet<DescID>&) const;
    DescID			getStoredID(const char* lk,int selout=0,
	    				    bool create=true);
    bool 			getFirstStored(Pol2D p2d, MultiID& key) const;
    Desc* 			getFirstStored(Pol2D p2d);

    void			removeDesc(const DescID&);
    void			removeAll();
    int                 	removeUnused(bool removestored=false);
				//!< Removes unused hidden attributes.
				//!< Removed stored attribs if not available
				//!< or if removestored flag is true;
				//!< Returns total removed.
    bool 			isAttribUsed(const DescID&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&,BufferStringSet* errmsgs=0);
    bool 			useOldSteeringPar(IOPar&,ObjectSet<Desc>&,
	    					  BufferStringSet*);
    bool			createSteeringDesc(const IOPar&,BufferString,
						   ObjectSet<Desc>&, int& id, 
						   BufferStringSet* errmsgs=0);
    Desc* 			createDesc(const BufferString&, const IOPar&,
	    				  const BufferString&,BufferStringSet*);
    bool 			setAllInputDescs(int, const IOPar&,
						 BufferStringSet*);
    void 			handleStorageOldFormat(IOPar&);
    void 			handleOldAttributes(BufferString&, IOPar&,
						    BufferString&);

    bool			is2D() const;
    const char*			errMsg() const;
    static const char*		highestIDStr()		{ return "MaxNrKeys"; }
    static const char*		definitionStr()		{ return "Definition"; }
    static const char*		userRefStr()		{ return "UserRef"; }
    static const char*		inputPrefixStr()	{ return "Input"; }
    static const char*		hiddenStr()		{ return "Hidden"; }

protected:

    DescID			getFreeID() const;

    ObjectSet<Desc>		descs;
    TypeSet<DescID>		ids;
    BufferString		errmsg;
    bool			firsttime;
};

} // namespace Attrib

#endif

