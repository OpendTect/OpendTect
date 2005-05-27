#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.7 2005-05-27 07:28:42 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"

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

    int				addDesc(Desc*);
				/*!<\returns id of the attrib */

    Desc*       		getDesc(int id);
    const Desc*			getDesc(int id) const;

    int				nrDescs() const;
    int				getID(const Desc&) const;
    int				getID(const char* ref,bool isusrref) const;
    void			getIds(TypeSet<int>&) const;

    void			removeDesc(int id);
    void			removeAll();

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&,BufferStringSet* errmsgs=0);
    bool			createSteeringDesc( const IOPar&,
						    ObjectSet<Desc>&, 
						    BufferStringSet* errmsgs=0);
	    

    bool			is2D() const;
    const char*			errMsg() const;

protected:

    int				getFreeID() const;
    static const char*		highestIDStr()		{ return "MaxNrKeys"; }
    static const char*		definitionStr()		{ return "Definition"; }
    static const char*		userRefStr()		{ return "UserRef"; }
    static const char*		inputPrefixStr()	{ return "Input"; }

    ObjectSet<Desc>		descs;
    TypeSet<int>		ids;
    BufferString		errmsg;
};

}; // namespace Attrib

#endif

