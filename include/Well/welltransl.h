#ifndef welltransl_h
#define welltransl_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: welltransl.h,v 1.2 2003-09-26 11:43:37 nanne Exp $
________________________________________________________________________


-*/

#include "transl.h"


namespace Well { class Data; };

class WellTranslator : public Translator
{			isTranslatorGroup(Well)
public:
    			WellTranslator( const char* nm = 0 )
			    : Translator( nm )			{}

    virtual		~WellTranslator() {}

    virtual bool	read(Well::Data&,const IOObj&)		{ return false;}
    virtual bool	write(const Well::Data&,const IOObj&)	{ return false;}

    static int			selector(const char*);
    static const IOObjContext&	ioContext();
    const char*			defExtension() const { return "well"; }

    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
	    				   const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

};


class dgbWellTranslator : public WellTranslator
{			  isTranslator(dgb,Well)
public:
    			dgbWellTranslator( const char* nm = 0 )
			    : WellTranslator(nm)		{}

    virtual bool	read(Well::Data&,const IOObj&);
    virtual bool	write(const Well::Data&,const IOObj&);

};


#endif
