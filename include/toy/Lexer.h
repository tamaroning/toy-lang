#ifndef TOY_LEXER_H
#define TOY_LEXER_H

#include "llvm/ADT/StringRef.h"

#include <memory>
#include <string>

namespace toy {

/// A location in a file
struct Location {
  /// filename
  std::shared_ptr<std::string> file;
  /// line number
  int line;
  /// column number
  int col;
};

/// List of Token returned by the lexer
enum Token : int {
  tok_semicolon = ';',
  tok_parenthese_open = '(',
  tok_parenthese_close = ')',
  tok_bracket_open = '{',
  tok_bracket_close = '}',
  tok_sbracket_open = '[',
  tok_sbracket_close = ']',

  tok_eof = -1,

  // commands
  tok_return = -2,
  tok_var = -3,
  tok_def = -4,

  // primary
  tok_identifier = -5,
  tok_number = -6,
};

class Lexer {
public:
  Lexer(std::string filename)
      : lastLocation(
            {std::make_shared<std::string>(std::move(filename)), 0, 0}) {}

  virtual ~Lexer() = default;

  Token getCurToken() { return curTok; }

  Token getNextToken() { return curTok = getTok(); }

  void consume(Token tok) {
    assert(tok == curTok && "consume Toke Mismatch expectation");
    getNextToken();
  }

  /// Return the current identifier (prereq: getCurToken() == tok_identifier)
  llvm::StringRef getId() {
    assert(curTok == tok_identifier);
    return identifierStr;
  }

  /// Return the current number (prereq: getCurToken() == tok_number)
  double getValue() {
    assert(curTok == tok_number);
    return numVal;
  }

  /// Return the location for the beginning of the current token
  Location getLastLocation() { return lastLocation; }

  /// Return the current line in the file
  int getLine() { return curLineNum; }

  /// Return the current column in the file
  int getCol() { return curCol; }

private:
  /// Fetch the next line. Returns an empty string to signal EOF.
  virtual llvm::StringRef readNextLine() = 0;

  /// Return the next character from the stream
  int getNextChar() {
    if (curLineBuffer.empty()) {
      return EOF;
    }
    ++curCol;
    auto nextchar = curLineBuffer.front();
    curLineBuffer = curLineBuffer.drop_front();
    if (curLineBuffer.empty()) {
      curLineBuffer = readNextLine();
    }
    if (nextchar == '\n') {
      ++curLineNum;
      curCol = 0;
    }
    return nextchar;
  }

  /// Return the next token from standard input
  Token getTok() {
    // skip whitespaces
    while (isspace(lastChar)) {
      lastChar = Token(getNextChar());
    }

    lastLocation.line = curLineNum;
    lastLocation.col = curCol;

    // Identifier: [a-zA-Z][a-zA-Z0-9_]*
    if (isalpha(lastChar)) {
      identifierStr = (char)lastChar;
      while (isalnum((lastChar = Token(getNextChar()))) || lastChar == '_') {
        identifierStr += (char)lastChar;
      }

      if (identifierStr == "return")
        return tok_return;
      if (identifierStr == "def")
        return tok_def;
      if (identifierStr == "var")
        return tok_var;
      return tok_identifier;
    }

    // Number: [0-9.]+
    if (isdigit(lastChar) || lastChar == '.') {
      std::string numStr;
      do {
        numStr += lastChar;
        lastChar = Token(getNextChar());
      } while (isdigit(lastChar) || lastChar == '.');

      numVal = strtod(numStr.c_str(), nullptr);
      return tok_number;
    }

    if (lastChar == '#') {
      // Comment until end of line.
      do {
        lastChar = Token(getNextChar());
      } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');

      if (lastChar != EOF)
        return getTok();
    }

    // Check for end of file.  Don't eat the EOF.
    if (lastChar == EOF)
      return tok_eof;

    // Otherwise, just return the character as its ascii value.
    Token thisChar = Token(lastChar);
    lastChar = Token(getNextChar());
    return thisChar;
  }

  /// The last token read from the input
  Token curTok = tok_eof;

  /// Location for `curTok`
  Location lastLocation;

  /// If the current Token is an identifier, this string contains the value.
  std::string identifierStr;

  /// If the current Token is a number, this contains the value.
  double numVal = 0;

  /// The last value returned by getNextChar()
  Token lastChar = Token(' ');

  /// The current line number in the input stream
  int curLineNum = 0;

  /// The current column number in the input stream
  int curCol = 0;

  /// Buffer supplied by the derived class on calls to `readNextLine()`
  llvm::StringRef curLineBuffer = "\n";
};

/// A lexer implementation operating on a buffer in memory
class LexerBuffer final : public Lexer {
public:
  LexerBuffer(const char *begin, const char *end, std::string filename)
      : Lexer(std::move(filename)), current(begin), end(end) {}

private:
  /// Provide one line at a time to the Lexer, return an empty string when
  /// reaching the end of the buffer.
  llvm::StringRef readNextLine() override {
    auto *begin = current;
    while (current <= end && *current && *current != '\n') {
      ++current;
    }
    if (current <= end && *current) {
      ++current;
    }
    llvm::StringRef result{begin, static_cast<size_t>(current - begin)};
    return result;
  }

  const char *current, *end;
};

} // namespace toy
#endif // TOY_LEXER_H