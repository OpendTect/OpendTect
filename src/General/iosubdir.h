#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    bool	implRename(const char*) override	{ return false; }
    bool	implSetReadOnly(bool) const override	{ return false; }

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
