#include "doxide/Parser.hpp"

const Node& Parser::root() const {
  return global;
}

void Parser::parse(const std::string& file) {
  tokenizer.load(file);

  Token token = consume(NAMESPACE|DOC);
  while (token.type && token.type & (NAMESPACE|DOC)) {
    global.add(parseEntity(token));
    token = consume(NAMESPACE|DOC);
  }
}

Node Parser::parseEntity(const Token& first) {
  static const auto useful =
      NAMESPACE|TYPE|TILDE|EQUALS|BRACE|SEMICOLON|PAREN|OPERATOR;

  Node node;
  Token from = first, scan = first;
  if (first.type & DOC) {
    /* consume the documentation comment first */
    node = interpret();
    if (node.type == NodeType::FILE) {  // skip @file
      return node;
    }
    from = consume(~WHITESPACE);
    scan = (from.type & useful) ? from : consume(useful);
  }

  if (scan.type & NAMESPACE) {
    /* namespace */
    node.type = NodeType::NAMESPACE;

    /* name */
    scan = consume(WORD);
    node.name = scan.str();

    /* signature */
    scan = consume(BRACE|SEMICOLON);
    node.decl = std::string_view(from.first, scan.first);

    /* members */
    if (scan.type & BRACE) {
      do {
        scan = consume(NAMESPACE|DOC|BRACE_CLOSE);
        if (scan.type & (NAMESPACE|DOC)) {
          node.add(parseEntity(scan));
        }
      } while (scan.type && !(scan.type & BRACE_CLOSE));
    }
  } else if (scan.type & TYPE) {
    /* type */
    node.type = NodeType::TYPE;

    /* name */
    scan = consume(WORD);
    node.name = scan.str();

    /* signature */
    scan = consume(BRACE|SEMICOLON);
    node.decl = std::string_view(from.first, scan.first);

    /* members */
    if (scan.type & BRACE) {
      do {
        scan = consume(DOC|BRACE_CLOSE);
        if (scan.type & DOC) {
          node.add(parseEntity(scan));
        }
      } while (scan.type && !(scan.type & BRACE_CLOSE));
    }
  } else if (scan.type & (EQUALS|BRACE|SEMICOLON)) {
    /* variable, e.g. int x; int x = 0; int x{0}; */
    node.type = NodeType::VARIABLE;
    node.name = word.str();
    node.decl = std::string_view(from.first, scan.first);

    if (scan.type & (EQUALS|BRACE)) {
      /* skip to the end of the statement */
      scan = consume(SEMICOLON);
    }
  } else if (scan.type & TILDE) {
    /* destructor */
    node.type = NodeType::FUNCTION;

    /* name */
    scan = consume(WORD);
    node.name = "~";
    node.name += scan.str();

    /* signature */
    scan = consume(SEMICOLON|BRACE|COLON);
    node.decl = std::string_view(from.first, scan.first);

    if (scan.type & BRACE) {
      /* skip over the body */
      scan = consume(BRACE_CLOSE);
    }
  } else if (scan.type & PAREN) {
    /* function */
    node.type = NodeType::FUNCTION;
    node.name = word.str();

    /* signature */
    scan = consume(PAREN_CLOSE);
    scan = consume(SEMICOLON|BRACE|COLON);
    node.decl = std::string_view(from.first, scan.first);

    if (scan.type & COLON) {
      /* skip the initializer list */
      scan = consume(BRACE);
    }
    if (scan.type & BRACE) {
      /* skip the body */
      scan = consume(BRACE_CLOSE);
    }
  } else if (scan.type & OPERATOR) {
    /* operator */
    node.type = NodeType::OPERATOR;
    node.name = scan.str();

    /* signature */
    scan = consume(SEMICOLON|BRACE);
    node.decl = std::string_view(from.first, scan.first);

    if (scan.type & BRACE) {
      /* skip the body */
      scan = consume(BRACE_CLOSE);
    }
  }

  return node;
}

Token Parser::consume(const uint64_t stop) {
  Token token = tokenizer.next();
  if (token.type & WORD) {
    word = token;
  }
  while (token.type && !(token.type & stop)) {
    if (token.type & (BRACE|BRACKET|PAREN)) {
      /* consume to a matching close, which is one left shift away */
      token = consume(token.type << 1);
    } else if (token.type & ANGLE) {
      /* ambiguity with operators < > <= >=, consume to any close */
      token = consume(BRACE_CLOSE|BRACKET_CLOSE|PAREN_CLOSE|ANGLE_CLOSE);
      if (!(token.type & ANGLE_CLOSE)) {
        /* unmatched close, treat as an operator not a delimiter */
        return token;
      }
    }
    token = tokenizer.next();
    if (token.type & WORD) {
      word = token;
    }
  }
  return token;
}

std::string_view Parser::consumeWord() {
  Token token;
  do {
    token = tokenizer.next();
  } while (token.type && (token.type & WHITESPACE));
  return token.str();
}

std::string_view Parser::consumeSentence() {
  Token first = tokenizer.next();
  while (first.type && (first.type & WHITESPACE)) {
    first = tokenizer.next();
  }
  Token last = first;
  while (last.type && !(last.type & (SENTENCE|DOC_CLOSE))) {
    last = tokenizer.next();
  } 
  return std::string_view(first.first, last.last);
}

std::string_view Parser::consumeParagraph() {
  Token first = tokenizer.next();
  while (first.type && (first.type & WHITESPACE)) {
    first = tokenizer.next();
  }
  Token last = first;
  while (last.type && !(last.type & (DOC_PARA|DOC_CLOSE))) {
    last = tokenizer.next();
  }
  return std::string_view(first.first, last.last);
}

Node Parser::interpret() {
  Node node;
  Token token = tokenizer.next();
  int indent = 0;
  while (token.type && !(token.type & DOC_CLOSE)) {
    if (token.type & DOC_COMMAND) {
      /* non-legacy commands */
      if (token.substr(1) == "param" ||
          token.substr(1) == "param[in]") {
        node.docs.append(":material-location-enter: **Parameter** `");
        node.docs.append(consumeWord());
        node.docs.append("`\n:   ");
      } else if (token.substr(1) == "param[out]") {
        node.docs.append(":material-location-exit: **Parameter** `");
        node.docs.append(consumeWord());
        node.docs.append("`\n:   ");
      } else if (token.substr(1) == "param[in,out]") {
        node.docs.append(":material-location-enter::material-location-exit: **Parameter** `");
        node.docs.append(consumeWord());
        node.docs.append("`\n:   ");
      } else if (token.substr(1) == "tparam") {
        node.docs.append(":material-code-tags: **Template parameter** `");
        node.docs.append(consumeWord());
        node.docs.append("`\n:   ");
      } else if (token.substr(1) == "p") {
        node.docs.append("`");
        node.docs.append(consumeWord());
        node.docs.append("`");
      } else if (token.substr(1) == "return") {
        node.docs.append(":material-location-exit: **Return**\n:   ");
      } else if (token.substr(1) == "pre") {
        node.docs.append(":material-check-circle-outline: **Pre-condition**\n:   ");
      } else if (token.substr(1) == "post") {
        node.docs.append(":material-check-circle-outline: **Post-condition**\n:   ");
      } else if (token.substr(1) == "throw") {
        node.docs.append(":material-alert-circle-outline: **Throw**\n:   ");
      } else if (token.substr(1) == "see") {
        node.docs.append(":material-eye-outline: **See**\n:   ");
      } else if (token.substr(1) == "anchor") {
        node.docs.append("<a name=\"");
        node.docs.append(consumeWord());
        node.docs.append("\"></a>");
      } else if (token.substr(1) == "note" ||
          token.substr(1) == "abstract" ||
          token.substr(1) == "info" ||
          token.substr(1) == "tip" ||
          token.substr(1) == "success" ||
          token.substr(1) == "question" ||
          token.substr(1) == "warning" ||
          token.substr(1) == "failure" ||
          token.substr(1) == "danger" ||
          token.substr(1) == "bug" ||
          token.substr(1) == "example" ||
          token.substr(1) == "quote") {
        node.docs.append("!!! ");
        node.docs.append(token.substr(1));
        indent += 4;
        node.docs.append(indent, ' ');
      } else if (token.substr(1) == "group") {
        Node group;
        group.type = NodeType::GROUP;
        group.name = consumeWord();
        node.add(group);
        node.docs.append(":material-view-module-outline: **Group** [");
        node.docs.append(group.name);
        node.docs.append("](");
        node.docs.append(group.name);
        node.docs.append("/)\n:   ");
      } else if (token.substr(1) == "ingroup") {
        node.ingroup = consumeWord();

      /* legacy commands */
      } else if (token.substr(1) == "returns" ||
          token.substr(1) == "result") {
        node.docs.append(":material-location-exit: **Return**\n:   ");
      } else if (token.substr(1) == "sa") {
        node.docs.append(":material-eye-outline: **See**\n:   ");
      } else if (token.substr(1) == "file") {
        node.type = NodeType::FILE;
      } else if (token.substr(1) == "internal") {
        node.hide = true;
      } else if (token.substr(1) == "brief" ||
          token.substr(1) == "short") {
        node.brief.append(consumeSentence());
      } else if (token.substr(1) == "e" ||
          token.substr(1) == "em" ||
          token.substr(1) == "a") {
        node.docs.append("*");
        node.docs.append(consumeWord());
        node.docs.append("*");
      } else if (token.substr(1) == "b") {
        node.docs.append("**");
        node.docs.append(consumeWord());
        node.docs.append("**");
      } else if (token.substr(1) == "c") {
        node.docs.append("`");
        node.docs.append(consumeWord());
        node.docs.append("`");
      } else if (token.substr(1) == "f$") {
        node.docs.append("$");
      } else if (token.substr(1) == "f[" ||
          token.substr(1) == "f]") {
        node.docs.append("$$");
      } else if (token.substr(1) == "li" ||
          token.substr(1) == "arg") {
        node.docs.append("  - ");
      } else if (token.substr(1) == "ref") {
        auto href = consumeWord();
        auto text = consumeWord();
        node.docs.append("[");
        node.docs.append(text);
        node.docs.append("](#");
        node.docs.append(href);
        node.docs.append(")");
      } else if (token.substr(1) == "code" ||
          token.substr(1) == "endcode" ||
          token.substr(1) == "verbatim" ||
          token.substr(1) == "endverbatim") {
        node.docs.append("```");
      } else if (token.substr(1) == "attention") {
        node.docs.append("!!! warning \"Attention\"\n");
        indent += 4;
        node.docs.append(indent, ' ');
      } else if (token.substr(1) == "todo") {
        node.docs.append("!!! example \"To-do\"\n");
        indent += 4;
        node.docs.append(indent, ' ');
      } else if (token.substr(1) == "remark") {
        node.docs.append("!!! quote \"Remark\"\n");
        indent += 4;
        node.docs.append(indent, ' ');

      /* unrecognized commands */
      } else {
        warn("unrecognized command: " << token.str());
        node.docs.append(token.str());
      }
    } else if (token.type & DOC_ESCAPE) {
      node.docs.append(token.substr(1));
    } else if (token.type & DOC_PARA) {
      node.docs.append("\n\n");
      indent = std::max(indent - 4, 0);
    } else if (token.type & DOC_LINE) {
      node.docs.append("\n");
      node.docs.append(indent, ' ');
    } else {
      node.docs.append(token.str());
    }
    token = tokenizer.next();
  }

  return node;
}
