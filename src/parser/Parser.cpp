#include "parser/Parser.hpp"
#include "parser/Tokenizer.hpp"
#include "parser/Language.hpp"

Parser::Parser() :
    parser(nullptr),
    query(nullptr) {
  /* parser */
  parser = ts_parser_new();
  ts_parser_set_language(parser, tree_sitter_cpp());

  /* query */
  uint32_t error_offset;
  TSQueryError error_type;
  query = ts_query_new(tree_sitter_cpp(), query_cpp, strlen(query_cpp),
      &error_offset, &error_type);
  if (error_type != TSQueryErrorNone) {
    std::string_view from(query_cpp + error_offset, 40);
    error("invalid query starting " << from << "...");
  }
}

Parser::~Parser() {
  ts_query_delete(query);
  ts_parser_delete(parser);
}

void Parser::parse(const std::string_view& source, Entity& global) {
  /* parser */
  ts_parser_reset(parser);
  TSTree *tree = ts_parser_parse_string(parser, NULL, source.data(),
      source.size());
  TSNode node = ts_tree_root_node(tree);

  /* initialize stacks */
  std::stack<uint32_t> starts, ends;
  std::stack<Entity> entities;
  starts.push(ts_node_start_byte(node));
  ends.push(ts_node_end_byte(node));
  entities.push(std::move(global));

  /* query */
  TSQueryCursor* cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, node);
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    Entity entity;
    uint64_t start = 0, middle = 0, end = 0;
    for (uint16_t i = 0; i < match.capture_count; ++i) {
      node = match.captures[i].node;
      uint32_t id = match.captures[i].index; 
      uint32_t length = 0;
      uint32_t k = ts_node_start_byte(node);
      uint32_t l = ts_node_end_byte(node);
      const char* name = ts_query_capture_name_for_id(query, id, &length);

      if (strncmp(name, "docs", length) == 0) {
        translate(source.substr(k, l - k), entity);
      } else if (strncmp(name, "name", length) == 0) {
        entity.name = source.substr(k, l - k);
      } else if (strncmp(name, "body", length) == 0) {
        middle = ts_node_start_byte(node);
      } else if (strncmp(name, "value", length) == 0) {
        middle = ts_node_start_byte(node);
      } else {
        start = ts_node_start_byte(node);
        end = ts_node_end_byte(node);
        middle = end;

        if (strncmp(name, "namespace", length) == 0) {
          entity.type = EntityType::NAMESPACE;
        } else if (strncmp(name, "type", length) == 0) {
          entity.type = EntityType::TYPE;
        } else if (strncmp(name, "variable", length) == 0) {
          entity.type = EntityType::VARIABLE;
        } else if (strncmp(name, "function", length) == 0) {
          entity.type = EntityType::FUNCTION;
        } else if (strncmp(name, "operator", length) == 0) {
          entity.type = EntityType::OPERATOR;
        } else if (strncmp(name, "enumerator", length) == 0) {
          entity.type = EntityType::ENUMERATOR;
        } else if (strncmp(name, "macro", length) == 0) {
          entity.type = EntityType::MACRO;
        } else {
          warn("unrecognized match");
        }
      }
    }

    /* entity declaration */
    entity.decl = source.substr(start, middle - start);

    /* the final node represents the whole entity, pop the stack until we find
     * its direct parent, as determined using nested byte ranges */
    while (!(starts.top() <= start && end <= ends.top())) {
      Entity top = std::move(entities.top());
      entities.pop();
      entities.top().add(top);
      starts.pop();
      ends.pop();
    }

    /* override the ingroup for class members, as they cannot be moved out */
    if (entities.top().type == EntityType::TYPE) {
      entity.ingroup.clear();
    }

    /* push to stack */
    entities.push(std::move(entity));
    starts.push(start);
    ends.push(end);
  }

  /* finalize */
  while (entities.size() > 1) {
    Entity top = std::move(entities.top());
    entities.pop();
    entities.top().add(top);
  }
  global = std::move(entities.top());
  entities.pop();
  ts_query_cursor_delete(cursor);
}

void Parser::translate(const std::string_view& comment, Entity& entity) {
  int indent = 0;
  Tokenizer tokenizer(comment);
  Token token = tokenizer.next();
  if (token.type & OPEN) {  // otherwise not a documentation comment
    token = tokenizer.next();
    while (token.type) {
      if (token.type & COMMAND) {
        /* non-legacy commands */
        if (token.substr(1) == "param" ||
            token.substr(1) == "param[in]") {
          entity.docs.append(":material-location-enter: **Parameter** `");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("`\n:   ");
          indent = 4;
        } else if (token.substr(1) == "param[out]") {
          entity.docs.append(":material-location-exit: **Parameter** `");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("`\n:   ");
          indent = 4;
        } else if (token.substr(1) == "param[in,out]") {
          entity.docs.append(":material-location-enter::material-location-exit: **Parameter** `");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("`\n:   ");
          indent = 4;
        } else if (token.substr(1) == "tparam") {
          entity.docs.append(":material-code-tags: **Template parameter** `");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("`\n:   ");
          indent = 4;
        } else if (token.substr(1) == "p") {
          entity.docs.append("`");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("`");
        } else if (token.substr(1) == "return") {
          entity.docs.append(":material-location-exit: **Return**\n:   ");
        } else if (token.substr(1) == "pre") {
          entity.docs.append(":material-check-circle-outline: **Pre-condition**\n:   ");
        } else if (token.substr(1) == "post") {
          entity.docs.append(":material-check-circle-outline: **Post-condition**\n:   ");
        } else if (token.substr(1) == "throw") {
          entity.docs.append(":material-alert-circle-outline: **Throw**\n:   ");
        } else if (token.substr(1) == "see") {
          entity.docs.append(":material-eye-outline: **See**\n:   ");
        } else if (token.substr(1) == "anchor") {
          entity.docs.append("<a name=\"");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("\"></a>");
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
          entity.docs.append("!!! ");
          entity.docs.append(token.substr(1));
          entity.docs.append("\n");
          indent += 4;
          entity.docs.append(indent, ' ');
        } else if (token.substr(1) == "ingroup") {
          entity.ingroup = tokenizer.consume(WORD).str();

        /* legacy commands */
        } else if (token.substr(1) == "returns" ||
            token.substr(1) == "result") {
          entity.docs.append(":material-location-exit: **Return**\n:   ");
        } else if (token.substr(1) == "sa") {
          entity.docs.append(":material-eye-outline: **See**\n:   ");
        } else if (token.substr(1) == "file") {
          entity.type = EntityType::FILE;
        } else if (token.substr(1) == "internal") {
          entity.hide = true;
        } else if (token.substr(1) == "brief" ||
            token.substr(1) == "short") {
          auto first = tokenizer.consume(~WHITESPACE);
          auto last = tokenizer.consume(SENTENCE);
          entity.brief.append(first.first, last.last);
        } else if (token.substr(1) == "e" ||
            token.substr(1) == "em" ||
            token.substr(1) == "a") {
          entity.docs.append("*");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("*");
        } else if (token.substr(1) == "b") {
          entity.docs.append("**");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("**");
        } else if (token.substr(1) == "c") {
          entity.docs.append("`");
          entity.docs.append(tokenizer.consume(WORD).str());
          entity.docs.append("`");
        } else if (token.substr(1) == "f$") {
          entity.docs.append("$");
        } else if (token.substr(1) == "f[" ||
            token.substr(1) == "f]") {
          entity.docs.append("$$");
        } else if (token.substr(1) == "li" ||
            token.substr(1) == "arg") {
          entity.docs.append("  - ");
        } else if (token.substr(1) == "ref") {
          auto href = tokenizer.consume(WORD);
          auto text = tokenizer.consume(WORD);
          entity.docs.append("[");
          entity.docs.append(text.str());
          entity.docs.append("](#");
          entity.docs.append(href.str());
          entity.docs.append(")");
        } else if (token.substr(1) == "code" ||
            token.substr(1) == "endcode" ||
            token.substr(1) == "verbatim" ||
            token.substr(1) == "endverbatim") {
          entity.docs.append("```");
        } else if (token.substr(1) == "attention") {
          entity.docs.append("!!! warning \"Attention\"\n");
          indent += 4;
          entity.docs.append(indent, ' ');
        } else if (token.substr(1) == "todo") {
          entity.docs.append("!!! example \"To-do\"\n");
          indent += 4;
          entity.docs.append(indent, ' ');
        } else if (token.substr(1) == "remark") {
          entity.docs.append("!!! quote \"Remark\"\n");
          indent += 4;
          entity.docs.append(indent, ' ');
        } else if (token.substr(1) == "def" ||
            token.substr(1) == "var" ||
            token.substr(1) == "fn" ||
            token.substr(1) == "class" ||
            token.substr(1) == "struct" ||
            token.substr(1) == "union" ||
            token.substr(1) == "enum" ||
            token.substr(1) == "typedef" ||
            token.substr(1) == "namespace" ||
            token.substr(1) == "interface" ||
            token.substr(1) == "protocol" ||
            token.substr(1) == "property") {
          /* ignore, including following name */
          tokenizer.consume(WORD);
        } else if (token.str().at(0) == '\\') {
          /* treat as escape */
          entity.docs.append(token.substr(1));
        } else {
          warn("unrecognized command: " << token.str());
          entity.docs.append(token.str());
        }
      } else if (token.type & ESCAPE) {
        entity.docs.append(token.substr(1));
      } else if (token.type & PARA) {
        entity.docs.append("\n\n");
        indent = std::max(indent - 4, 0);
      } else if (token.type & LINE) {
        entity.docs.append("\n");
        entity.docs.append(indent, ' ');
      } else if (token.type & CLOSE) {
        //
      } else {
        entity.docs.append(token.str());
      }
      token = tokenizer.next();
    }
  }
}