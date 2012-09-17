#ifndef identifierman_h
#define identifierman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          March 2011
 RCS:           $Id: identifierman.h,v 1.1 2012-09-17 12:38:32 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "objectset.h"
#include "separstr.h"


namespace CmdDrive
{

mClass(CmdDriver) IdentifierManager
{
public:
    			IdentifierManager();
			~IdentifierManager();

    void		reInit();
    void		raiseScopeLevel(bool up=true);

    void    		set(const char* name,const char* val="",
			    bool islink=false);
    void		set(const char* name,int);
    void    		set(const char* name,double);
    void		unset(const char* name,bool followlinks=true);

    bool		isPredefined(const char* name) const;
    bool		doesExist(const char* name) const;
    const char*		getValue(const char* name) const;

    bool		getInteger(const char* name,int&) const;
    bool		getDouble(const char* name,double&) const;

    const char*		lastLinkedIdentStr() const;

    int			substitute(const char* src,BufferString& dest);
			// Returns failed (<0) or successful (>=0) nr. subst

protected:

    class Identifier
    {
	public:
			Identifier(const char* name,const char* val="",
				   bool islink=false)
			    : name_(name), val_(val), islink_(islink)
			    , predefined_(false)
			    , filepathplaceholder_(false)
			    , refresh_(false)				{} 

	BufferString	name_;
	BufferString	val_;

	bool		islink_;
	bool		predefined_;
	bool		filepathplaceholder_;
	bool		refresh_;
    };

    bool		findCurIdent(const char* name,bool followlinks=true,
	    			     bool singlescope=false);
    void    		setFilePathPlaceholder(const char* nm,const char* val);

    Identifier*		curident_; // Always points to last identifier set/read
    int			curlevel_; // Always points to last level searched

    SeparString		lastlinkedidentstr_;

    ObjectSet< ObjectSet<Identifier> > identifiers_;
};


}; // namespace CmdDrive


#endif

