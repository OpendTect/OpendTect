#ifndef picksettr_h
#define picksettr_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: picksettr.h,v 1.1 2003-09-16 11:40:51 bert Exp $
________________________________________________________________________

@$*/
 
#include <transl.h>
#include <ctxtioobj.h>
class Conn;
class PickSetGroup;


class PickSetGroupTranslator : public Translator
{			  isTranslatorGroup(PickSetGroup)
public:
			PickSetGroupTranslator( const char* nm = 0 )
			: Translator(nm)		{}
    virtual		~PickSetGroupTranslator()	{}

    virtual const char*	read(PickSetGroup&,Conn&,const bool* selarr=0)
							{ return "err"; }
			//!< returns err msg or null on success
    virtual const char*	write(const PickSetGroup&,Conn&,const bool* selarr=0)
							{ return "err"; }
			//!< returns err msg or null on success

    static bool		retrieve(PickSetGroup&,const IOObj*,BufferString&,
				const bool* selarr=0);
    static bool		store(const PickSetGroup&,const IOObj*,BufferString&,
				const bool* selarr=0,bool domrg=false);
    			//!< if domrg == true, if set already exists new set
			//!< will be merged

    static int		selector(const char*);
    static const IOObjContext&	ioContext();
    virtual const char*	defExtension() const		{ return "pck"; }
};



class dgbPickSetGroupTranslator : public PickSetGroupTranslator
{			     isTranslator(dgb,PickSetGroup)
public:
			dgbPickSetGroupTranslator( const char* nm = 0 )
			: PickSetGroupTranslator(nm)		{}

    const char*		read(PickSetGroup&,Conn&,const bool* s=0);
    const char*		write(const PickSetGroup&,Conn&,const bool* s=0);

};


#endif
