#include "markdown/MarkdownGenerator.hpp"

void MarkdownGenerator::generate(const std::filesystem::path& dir,
    const Entity& global) {
  generateGroup(dir, global);
}

void MarkdownGenerator::generateGroup(const std::filesystem::path& dir,
    const Entity& node) {
  std::ofstream out;
  if (node.name.empty()) {
    std::filesystem::create_directories(dir);
    out.open(dir / "index.md", std::ios::app);
  } else {
    std::filesystem::create_directories(dir / sanitize(node.name));
    out.open(dir / sanitize(node.name) / "index.md");

    /* use YAML frontmatter to ensure correct capitalization of the title */
    out << "title: " << node.name << std::endl;
    out << "---" << std::endl;
    out << std::endl;

    /* header */
    out << "# " << node.name << std::endl;
    out << std::endl;
  }

  /* groups */
  if (node.groups.size() > 0) {
    out << "## Groups" << std::endl;
    for (auto& [name, child] : node.groups) {
      out << ":material-view-module-outline: [" << name << "](" << sanitize(name) << "/)" << std::endl;
      out << ":   " << child.docs << std::endl;
      out << std::endl;
    }
  }

  if (node.type == EntityType::NAMESPACE) {
    /* namespace page */
    out << "**" << htmlize(line(node.decl)) << "**" << std::endl;
    out << std::endl;
    out << node.docs << std::endl;
    out << std::endl;
  }

  if (node.namespaces.size() > 0) {
    out << "## Namespaces" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.namespaces) {
      out << "| [" << name << "](" << sanitize(name) << "/) | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }
  if (node.macros.size() > 0) {
    out << "## Macros" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.macros) {
      out << "| [" << name << "](" << sanitize(name) << "/) | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }
  if (node.types.size() > 0) {
    out << "## Types" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.types) {
      out << "| [" << name << "](types/" << sanitize(name) << "/) | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }
  if (node.variables.size() > 0) {
    out << "## Variables" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.variables) {
      out << "| [" << name << "](variables/" << sanitize(name) << "/) | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }
  if (node.operators.size() > 0) {
    out << "## Operators" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.operators) {
      out << "| [" << name << "](operators/" << sanitize(name) << "/) | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }
  if (node.functions.size() > 0) {
    out << "## Functions" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.functions) {
      out << "| [" << name << "](functions/" << sanitize(name) << "/) | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }

  /* child pages */
  for (auto& [name, child] : node.groups) {
    generateGroup(dir / sanitize(node.name), child);
  }
  for (auto& [name, child] : node.namespaces) {
    generateGroup(dir / sanitize(node.name), child);
  }
  for (auto& [name, child] : node.macros) {
    generateMacro(dir / sanitize(node.name), child);
  }
  for (auto& [name, child] : node.types) {
    generateType(dir / sanitize(node.name) / "types", child);
  }
  for (auto& [name, child] : node.variables) {
    generateVariable(dir / sanitize(node.name) / "variables", child);
  }
  for (auto iter = node.operators.begin(); iter != node.operators.end(); ) {
    auto first = iter;
    do {
      ++iter;
    } while (iter != node.operators.end() && iter->first == first->first);
    generateOperator(dir / sanitize(node.name) / "operators", first, iter);
  }
  for (auto iter = node.functions.begin(); iter != node.functions.end(); ) {
    auto first = iter;
    do {
      ++iter;
    } while (iter != node.functions.end() && iter->first == first->first);
    generateFunction(dir / sanitize(node.name) / "functions", first, iter);
  }
}

void MarkdownGenerator::generateMacro(const std::filesystem::path& dir,
    const Entity& node) {
  std::filesystem::create_directories(dir);
  std::ofstream out(dir / (sanitize(node.name) + ".md"));

  /* use YAML frontmatter to ensure correct capitalization of the title */
  out << "title: " << node.name << std::endl;
  out << "---" << std::endl;
  out << std::endl;

  out << "# " << node.name << std::endl;
  out << std::endl;
  out << "!!! macro \"" << htmlize(line(node.decl)) << '"' << std::endl;
  out << std::endl;
  out << indent(node.docs) << std::endl;
  out << std::endl;
}

void MarkdownGenerator::generateType(const std::filesystem::path& dir,
    const Entity& node) {
  std::filesystem::create_directories(dir);
  std::ofstream out(dir / (sanitize(node.name) + ".md"));

  /* use YAML frontmatter to ensure correct capitalization of the title */
  out << "title: " << node.name << std::endl;
  out << "---" << std::endl;
  out << std::endl;

  /* type page */
  out << "# " + node.name << std::endl;
  out << std::endl;
  out << "**" << htmlize(line(node.decl)) << "**" << std::endl;
  out << std::endl;
  out << node.docs << std::endl;
  out << std::endl;

  /* for an enumerator, output the possible values */
  if (node.enumerators.size() > 0) {
    for (auto& [name, child] : node.enumerators) {
      out << "**" << name << "**" << std::endl;
      out << ":   " << child.docs << std::endl;
      out << std::endl;
    }
    out << std::endl;
  }

  /* brief descriptions */
  if (node.variables.size() > 0) {
    out << "## Member Variables" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.variables) {
      out << "| [" << name << "](#" << sanitize(name) << ") | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }
  if (node.operators.size() > 0) {
    out << "## Member Operators" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.operators) {
      out << "| [" << name << "](#" << sanitize(name) << ") | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }
  if (node.functions.size() > 0) {
    out << "## Member Functions" << std::endl;
    out << std::endl;
    out << "| Name | Description |" << std::endl;
    out << "| ---- | ----------- |" << std::endl;
    for (auto& [name, child] : node.functions) {
      out << "| [" << name << "](#" << sanitize(name) << ") | ";
      out << brief(child.docs) << " |" << std::endl;
    }
    out << std::endl;
  }

  /* detailed descriptions */
  if (node.variables.size() > 0) {
    out << "## Member Variable Details" << std::endl;
    out << std::endl;
    for (auto& [name, child] : node.variables) {
      //out << "### " << name;
      out << "<a name=\"" << sanitize(name) << "\"></a>" << std::endl;
      out << std::endl;
      out << "!!! variable \"" << htmlize(line(child.decl)) << '"' << std::endl;
      out << std::endl;
      out << indent(child.docs) << std::endl;
      out << std::endl;
    }
  }
  if (node.operators.size() > 0) {
    out << "## Member Operator Details" << std::endl;
    out << std::endl;
    std::string prev;
    for (auto& [name, child] : node.operators) {
      if (name != prev) {
        /* heading only for the first overload of this name */
        //out << "### " << name;
        out << "<a name=\"" << sanitize(name) << "\"></a>" << std::endl;
        out << std::endl;
      }
      out << "!!! function \"" << htmlize(line(child.decl)) << '"' << std::endl;
      out << std::endl;
      out << indent(child.docs) << std::endl;
      out << std::endl;
      prev = name;
    }
  }
  if (node.functions.size() > 0) {
    out << "## Member Function Details" << std::endl;
    out << std::endl;
    std::string prev;
    for (auto& [name, child] : node.functions) {
      if (name != prev) {
        /* heading only for the first overload of this name */
        //out << "### " << name;
        out << "<a name=\"" << sanitize(name) << "\"></a>" << std::endl;
      }
      out << "!!! function \"" << htmlize(line(child.decl)) << '"' << std::endl;
      out << std::endl;
      out << indent(child.docs) << std::endl;
      out << std::endl;
      prev = name;
    }
  }
}

void MarkdownGenerator::generateVariable(const std::filesystem::path& dir,
    const Entity& node) {
  std::filesystem::create_directories(dir);
  std::ofstream out(dir / (sanitize(node.name) + ".md"));

  /* use YAML frontmatter to ensure correct capitalization of the title */
  out << "title: " << node.name << std::endl;
  out << "---" << std::endl;
  out << std::endl;

  out << "# " << node.name << std::endl;
  out << std::endl;
  out << "!!! variable \"" << htmlize(line(node.decl)) << '"' << std::endl;
  out << std::endl;
  out << indent(node.docs) << std::endl;
  out << std::endl;
}

template<class Iterator>
void MarkdownGenerator::generateFunction(const std::filesystem::path& dir,
    const Iterator& first, const Iterator& last) {
  auto& node = first->second;
  std::filesystem::create_directories(dir);
  std::ofstream out(dir / (sanitize(node.name) + ".md"));

  /* use YAML frontmatter to ensure correct capitalization of the title */
  out << "title: " << node.name << std::endl;
  out << "---" << std::endl;
  out << std::endl;

  out << "# " << node.name << std::endl;
  out << std::endl;
  for (auto iter = first; iter != last; ++iter) {
    auto& node = iter->second;
    out << "!!! function \"" << htmlize(line(node.decl)) << '"' << std::endl;
    out << std::endl;
    out << indent(node.docs) << std::endl;
    out << std::endl;
  }
}

template<class Iterator>
void MarkdownGenerator::generateOperator(const std::filesystem::path& dir,
    const Iterator& first, const Iterator& last) {
  auto& node = first->second;
  std::filesystem::create_directories(dir);
  std::ofstream out(dir / (sanitize(node.name) + ".md"));

  /* use YAML frontmatter to ensure correct capitalization of the title */
  out << "title: " << node.name << std::endl;
  out << "---" << std::endl;
  out << std::endl;

  out << "# " << node.name << std::endl;
  out << std::endl;
  for (auto iter = first; iter != last; ++iter) {
    auto& node = iter->second;
    out << "!!! function \"" << htmlize(line(node.decl)) << '"' << std::endl;
    out << std::endl;
    out << indent(node.docs) << std::endl;
    out << std::endl;
  }
}

std::string MarkdownGenerator::brief(const std::string& str) {
  static const std::regex reg("^.*?[\\.\\?\\!]");

  std::string l = line(str);
  std::smatch match;
  if (std::regex_search(l, match, reg)) {
    return match.str();
  } else {
    return "";
  }
}

std::string MarkdownGenerator::line(const std::string& str) {
  static const std::regex newline("\\s*\\n\\s*");
  return std::regex_replace(str, newline, " ");
}

std::string MarkdownGenerator::indent(const std::string& str) {
  static const std::regex start("\\n");
  return "    " + std::regex_replace(str, start, "\n    ");
}

std::string MarkdownGenerator::htmlize(const std::string& str) {
  static const std::regex amp("&");
  static const std::regex lt("<");
  static const std::regex gt(">");
  static const std::regex quot("\"");
  static const std::regex apos("'");

  std::string r = str;
  r = std::regex_replace(r, amp, "&amp;");  // must go first or new & replaced
  r = std::regex_replace(r, lt, "&lt;");
  r = std::regex_replace(r, gt, "&gt;");
  r = std::regex_replace(r, quot, "&quot;");
  r = std::regex_replace(r, apos, "&apos;");
  return r;
}

std::string MarkdownGenerator::sanitize(const std::string& str) {
  static const std::regex word("\\w");

  std::stringstream buf;
  for (auto iter = str.begin(); iter != str.end(); ++iter) {
    if (std::regex_match(iter, iter + 1, word)) {
      buf << *iter;
    } else {
      buf << "_u" << std::setfill('0') << std::setw(4) << std::hex << int(*iter);
    }
  }
  return buf.str();
}