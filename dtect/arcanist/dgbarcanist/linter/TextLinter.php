<?php

include_once( "TabTools.php" );

/**
 * Enforces basic text file rules.
 *
 * @group linter
 */
final class TextLinter extends ArcanistLinter {

  const LINT_DOS_NEWLINE		= 1;
  const LINT_TAB_LITERAL		= 2;
  const LINT_LINE_WRAP			= 3;
  const LINT_EOF_NEWLINE		= 4;
  const LINT_BAD_CHARSET		= 5;
  const LINT_TRAILING_WHITESPACE	= 6;
  const LINT_NO_COMMIT			= 7;
  const LINT_SPACE_ALIGNMENT		= 8;
  const LINT_FORBIDDEN_WORD		= 9;
  const LINT_TR_IN_MACRO		= 11;
  const LINT_NEWLINES_BEFORE_EOF	= 12;

  private $maxLineLength = 80;
  private $nrautofixes = 0;

  public function getLinterPriority() {
    return 0.5;
  }

  public function setMaxLineLength($new_length) {
    $this->maxLineLength = $new_length;
    return $this;
  }

  public function getLinterName() {
    return 'TXT';
  }

  public function getLinterConfigurationName() {
    return 'text';
  }

  public function getLintSeverityMap() {
    if ( $this->nrautofixes>1 )
      return array();

    return array(
      self::LINT_TRAILING_WHITESPACE => ArcanistLintSeverity::SEVERITY_AUTOFIX,
      self::LINT_SPACE_ALIGNMENT     => ArcanistLintSeverity::SEVERITY_AUTOFIX,
      self::LINT_NEWLINES_BEFORE_EOF => ArcanistLintSeverity::SEVERITY_AUTOFIX
    );
  }

  public function getLintNameMap() {
    return array(
      self::LINT_DOS_NEWLINE		=> pht('DOS Newlines'),
      self::LINT_TAB_LITERAL		=> pht('Tab Literal'),
      self::LINT_LINE_WRAP		=> pht('Line Too Long'),
      self::LINT_EOF_NEWLINE		=> pht('File Does Not End in Newline'),
      self::LINT_BAD_CHARSET		=> pht('Bad Charset'),
      self::LINT_TRAILING_WHITESPACE	=> pht('Trailing Whitespace'),
      self::LINT_NO_COMMIT		=> pht('Explicit %s', '@no'.'commit'),
      self::LINT_SPACE_ALIGNMENT	=> pht('Spaces used instead of tabs'),
      self::LINT_FORBIDDEN_WORD 	=> pht('Forbidden words'),
      self::LINT_TR_IN_MACRO 		=> pht('tr() statement in macro'),
      self::LINT_NEWLINES_BEFORE_EOF 	=> pht('Empty lines at end of file'),
    );
  }

  public function lintPath($path) {
    $this->nrautofixes = 0;
    if (!strlen($this->getData($path))) {
      // If the file is empty, don't bother; particularly, don't require
      // the user to add a newline.
      return;
    }

    $iscmake = strpos( $path, ".cmake" )!==false ||
	       strpos( $path, ".txt") !==false;
    $isphp = strpos( $path, ".php" )!==false;

    $this->lintNewlines($path);

    if ($this->didStopAllLinters()) {
      return;
    }

    if ( !$isphp && !$iscmake ) {
      $this->lintForbiddenStrings($path);

      if ($this->didStopAllLinters()) {
	return;
      }

    }

    $this->lintCharset($path);

    if ($this->didStopAllLinters()) {
      return;
    }

    $this->lintEOFNewline($path);
    $this->lintTrailingWhitespace($path);
    $this->lintNewlineAtEOF($path);
    $this->lintTrInMacro($path);

    if (!$iscmake) {
      $this->lintLineLength($path);

      if ($this->didStopAllLinters()) {
	return;
      }
    }

    if ( !$isphp ) {
      $this->lintSpaceAlignment($path);
    }
  }

  protected function lintNewlines($path) {
    $pos = strpos($this->getData($path), "\r");
    if ($pos !== false) {
      $this->raiseLintAtOffset(
        $pos,
        self::LINT_DOS_NEWLINE,
        'You must use ONLY Unix linebreaks ("\n") in source code.',
        "\r");
      if ($this->isMessageEnabled(self::LINT_DOS_NEWLINE)) {
        $this->stopAllLinters();
      }
    }
  }

  protected function lintLineLength($path) {
    $lines = explode("\n", $this->getData($path));

    $width = $this->maxLineLength;
    foreach ($lines as $line_idx => $line) {

      $expandedline = tab_expand( $line );
      if (strlen($expandedline) > $width) {

      $isrcs = strpos( $expandedline, '$Id' );

      if ( $isrcs==false ) {
	$this->raiseLintAtLine(
	  $line_idx + 1,
	  1,
	  self::LINT_LINE_WRAP,
	  'This line is '.number_format(strlen($expandedline)).
	  ' characters long, '.
	  'but the convention is '.$width.' characters.',
	  $expandedline);

	  if ($this->isMessageEnabled(self::LINT_LINE_WRAP)) {
	    $this->stopAllLinters();
	  }
	}
      }
    }
  }


  protected function lintEOFNewline($path) {
    $data = $this->getData($path);
    if (!strlen($data) || $data[strlen($data) - 1] != "\n") {
      $this->raiseLintAtOffset(
        strlen($data),
        self::LINT_EOF_NEWLINE,
        "Files must end in a newline.",
        '',
        "\n");

      if ($this->isMessageEnabled(self::LINT_EOF_NEWLINE)) {
        $this->stopAllLinters();
      }
    }
  }


  protected function lintNewlineAtEOF($path) {
    $data = $this->getData($path);
     
    for ( $idx=strlen($data)-2; $idx>=0; $idx-- )
    {
	if ( $data[$idx]!="\n" )
	{
	    if ( $idx<strlen($data)-2 )
	    {
	      $this->raiseLintAtOffset(
		$idx+2,
		self::LINT_NEWLINES_BEFORE_EOF,
		'There are empty lines at end of file. Please remove.',
		substr( $data, $idx+1 ),
		"");

		$this->nrautofixes++;
		return;
	    }
	    break;
	}
    }
  }

  protected function lintCharset($path) {
    $data = $this->getData($path);

    $matches = null;
    $bad = '[^\x09\x0A\x0D\x20-\x7E]';
    $preg = preg_match_all(
      "/{$bad}(.*{$bad})?/",
      $data,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg) {
      return;
    }

    foreach ($matches[0] as $match) {
      list($string, $offset) = $match;
      $this->raiseLintAtOffset(
        $offset,
        self::LINT_BAD_CHARSET,
        'Source code should contain only ASCII bytes with ordinal decimal '.
        'values between 32 and 126 inclusive, plus linefeed. Do not use UTF-8 '.
        'or other multibyte charsets.',
        $string);

      if ($this->isMessageEnabled(self::LINT_BAD_CHARSET)) {
        $this->stopAllLinters();
      }
    }
  }

  protected function lintForbiddenStrings($path) {

    $badstrings = array( 'sqrt(', 'atan2(' );
    $badregexps = array( '/[\n[:blank:]]+small+[^a-zA-Z0-9\s]+/' );
// We have cases that require detachAllNotifers() not being the first line.
//			 '/[;]+[\n[:blank:]]+detachAllNotifiers/' );
    $regexpmsgs = array( 'Variables are not allowed to be called '.
			 'small as it interferes with types on some compliers',
			 'detachAllNotifiers() must be the first call in the '.
			 'destructor' );
    //$bad = array( 'sqrt', 'std::ostream', 'std::istream' );

    $data = $this->emptyDoubleQuotes( $this->getData( $path ) );

    foreach ( $badstrings as $keyword ) {
      $offset = strpos( $data, $keyword );
      if ($offset !== false) {
	$this->raiseLintAtOffset(
	  $offset,
	  self::LINT_FORBIDDEN_WORD,
	  'This file contains the string "'.$keyword.'", which is on the '.
	  'list of words not allowed in the code' );

	if ($this->isMessageEnabled(self::LINT_FORBIDDEN_WORD) ) {
	  $this->stopAllLinters();
	}
      }
    }

    foreach ( $badregexps as $index => $keyword ) {
      $matches = null;
      $preg = preg_match_all(
	$keyword,
	$data,
	$matches,
	PREG_OFFSET_CAPTURE);

      if (!$preg) {
	continue;
      }

      foreach ($matches[0] as $match) {
	list($string, $offset) = $match;
	if ( $regexpmsgs[$index]== '' )
	{
	    $message =
	      'This file contains the string "'.$string.'", which is on the '.
	      'list of words not allowed in the code';
	}
	else
	{
	   $message = $regexpmsgs[$index];
        }
	$this->raiseLintAtOffset(
	  $offset,
	   self::LINT_FORBIDDEN_WORD, $message );
      }

      if ($this->isMessageEnabled(self::LINT_FORBIDDEN_WORD) ) {
	$this->stopAllLinters();
      }
    }
  }

  protected function lintSpaceAlignment($path) {
    $lines = explode("\n", $this->getData($path));

    $change = false;
    foreach ($lines as $line_idx => $line) {

      $expanded = tab_expand($line);
      $changedline = tab_collapse( $expanded, 8 );
      if ( strcmp( $line, $changedline ) ) {
	$this->raiseLintAtLine(
		$line_idx + 1,
		1,
		self::LINT_SPACE_ALIGNMENT,
		'This line uses spaces in stead of tabs',
		$line,
		$changedline );
	$change = true;
      }
    }

    if ( $change )
      $this->nrautofixes++;
  }

  protected function lintTrailingWhitespace($path) {
    $data = $this->getData($path);

    $matches = null;
    $preg = preg_match_all(
      '/[[:blank:]]+$/m',
      $data,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg) {
      return;
    }

    foreach ($matches[0] as $match) {
      list($string, $offset) = $match;
      $this->raiseLintAtOffset(
        $offset,
        self::LINT_TRAILING_WHITESPACE,
        'This line contains trailing whitespace. Consider setting up your '.
          'editor to automatically remove trailing whitespace, you will save '.
          'time.',
        $string,
        '');
    }

    $this->nrautofixes++;
  }


  protected function lintTrInMacro($path) {
    //Make macros to one line
    $data = str_replace( "\\\n", "  ", $this->getData($path) );

    $matches = null;
    $preg = preg_match_all(
      '/#define[^\n]+[^a-zA-Z_]+tr[[:blank:]]*\([[:blank:]]*"/m',
      $data,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg) {
      return;
    }

    foreach ($matches[0] as $match) {
      list($string, $offset) = $match;
      $this->raiseLintAtOffset(
        $offset,
        self::LINT_TR_IN_MACRO,
        'This line a tr() call inside a macro, which is not allowed.' );
    }
  }

  private function lintNoCommit($path) {
    $data = $this->getData($path);

    $deadly = '@no'.'commit';

    $offset = strpos($data, $deadly);
    if ($offset !== false) {
      $this->raiseLintAtOffset(
        $offset,
        self::LINT_NO_COMMIT,
        'This file is explicitly marked as "'.$deadly.'", which blocks '.
        'commits.',
        $deadly);
    }
  }

  //Replaces quoted text with space-string
  private function emptyDoubleQuotes($data) { 
    //Remove everything inside double-quotes
    $preg = preg_match_all(
      '/"[^"]*["]+/',
      $data,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg)
      return $data;

    foreach ($matches[0] as $match) {
      list($string, $offset) = $match;
      $len = strlen( $string );
      if ( $len>2 ) {
	$repl = str_pad( "", $len-2 );
	$repl = "\"".$repl."\"";
      
	$data = substr_replace( $data, $repl, $offset, $len );
      }
    }

    return $data;
  }
}

