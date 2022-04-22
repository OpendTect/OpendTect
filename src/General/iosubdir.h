#ifndef iosubdir_h
#define iosubdir_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2010
________________________________________________________________________

*/

#include "ioobj.h"


class IOSubDir : public IOObj
{
public:
		IOSubDir(const char* subdirnm);
		IOSubDir(const IOSubDir&);
    bool	isSubdir() const override	{ return true; }
    bool	isBad() const override		{ return isbad_; }
    void	copyFrom(const IOObj*) override {}
    static IOSubDir* get(ascistream&,const char* rootdirnm);

    const char* fullUserExpr(bool) const override;
    bool	implExists(bool) const override { return !isbad_; }
    bool	implReadOnly() const override	{ return false; }
    bool	implRemove() const override	{ return false; }
    bool	implManagesObjects() const override { return true; }
    bool	implRename(const char*,const CallBack*) override
					{ return false; }
    bool	implSetReadOnly(bool) const override { return false; }

    const char* connType() const override	{ return 0; }
    Conn*	getConn( bool forread ) const override { return 0; }

    const char* dirName() const override	{ return fullUserExpr(true); }
    int		myKey() const override		{ return key_.groupID(); }

protected:

    bool	isbad_;

    bool	getFrom(ascistream&) override	{ return true; }
    bool	putTo(ascostream&) const override;

   friend class	IOMan;

};


#endif
