#ifndef stratlevel_h
#define stratlevel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratlevel.h,v 1.1 2010-09-08 15:13:54 cvsbert Exp $
________________________________________________________________________

-*/

#include "undefval.h"
#include "namedobj.h"
#include "ranges.h"
#include "color.h"
class IOPar;
class BufferStringSet;

namespace Strat
{

/*!\brief Stratigraphic level

  A Level is used to tie a horizon and a well marker together. 
  To store extra details, use the pars_.

*/


mClass Level : public NamedObject
{
public:
			Level(const char* nm);
			Level(const Level&);
    virtual		~Level();
    Level&		operator =(const Level&);

    // Properties
    int			id_;
    Color		color_;
    IOPar&		pars_;

    void		putTo(IOPar&) const;
    void		getFrom(const IOPar&);
};


mClass LevelSet : public ObjectSet<Level>
{
public:
			LevelSet() 	
			    : lastlevelid_(-1)
			    {};

    bool 		isPresent(const char*) const;
    int			indexOf(const char*) const;
    
    const Level* 	getByName(const char*) const;
    const Level* 	getByID(int) const;
    
    void		addToList(const BufferStringSet&,const TypeSet<Color>&);
    int 		addToList(const char*,const Color&);

    int                 getNewID() const   	{ return ++lastlevelid_; }
    void		constraintID(int id)   	
    			{ if ( id > lastlevelid_ ) lastlevelid_ = id; }

protected:

    void 		insert(const char*,const Color&,int);
    void 		add(const BufferStringSet&,const TypeSet<Color>&);
    int 		add(const char*,const Color&);

    mutable int         lastlevelid_;

};

}; //namespace

#endif
