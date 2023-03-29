/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "nrbytes2string.h"
#include "testprog.h"
#include "globexpr.h"

#include <regex>
#include <QRegularExpression>


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( quiet_ )
	return 0;

    if ( argc < 3 )
	return 1;

    const std::string tomatch = argv[1];
    const std::string expr = argv[2];
    const bool casesensitive = argc < 4;

    const GlobExpr ge( expr.c_str(), casesensitive );

    od_cout() << "'" << tomatch.c_str()
	<< (ge.matches(tomatch.c_str()) ? "' matches '" : "' doesn't match '")
	<< expr.c_str() << "'" << od_endl;

    const std::regex stdge = casesensitive
			   ? std::regex( expr )
			   : std::regex( expr, std::regex::icase );
    std::smatch m;
    const bool stdmatch = std::regex_search( tomatch, m, stdge );
    od_cout() << "'" << tomatch.c_str()
	<< (stdmatch ? "' matches '" : "' doesn't match '")
	<< expr.c_str() << "'" << od_endl;

    const QString qexpr = expr.c_str();
    const QString qmatch = tomatch.c_str();
    const QRegularExpression qge( qexpr, casesensitive
			? QRegularExpression::NoPatternOption
			: QRegularExpression::CaseInsensitiveOption );
    const QRegularExpressionMatch qtmatch = qge.match( qmatch );
    const bool hasqtmatch = qtmatch.hasMatch();
    od_cout() << "'" << tomatch.c_str()
	<< (hasqtmatch ? "' matches '" : "' doesn't match '")
	<< expr.c_str() << "'" << od_endl;

    return 0;
}
