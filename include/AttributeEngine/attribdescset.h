#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.2 2005-02-03 15:35:02 kristofer Exp $
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
    DescSet*    clone() const;

    int		addDesc( Desc* );
		/*!<\returns id of the attrib */

    Desc*       getDesc(int id);
    const Desc* getDesc(int id) const;

    int		nrDescs() const;
    int		getID(const Desc&) const;
    void	getIds( TypeSet<int>& ) const;

    void	removeDesc(int id);
    void	removeAll();

    void	fillPar( IOPar& ) const;
    bool	usePar( const IOPar&, BufferStringSet* errmsgs = 0 );

    const char*	errMsg() const;

protected:
    int			getFreeID() const;
    static const char*	highestIDStr() { return "MaxNrKeys"; }
    static const char*	definitionStr() { return "Definition"; }
    static const char*	userRefStr() { return "UserRef"; }
    static const char*	inputPrefixStr() { return "Input."; }



    ObjectSet<Desc>	descs;
    TypeSet<int>	ids;
    BufferString	errmsg;
};

}; //Namespace

#endif

