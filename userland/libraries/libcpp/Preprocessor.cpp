/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Preprocessor.h"
#include <base/Assertions.h>
#include <base/GenericLexer.h>
#include <base/StringBuilder.h>
#include <libcpp/Lexer.h>
#include <ctype.h>

namespace Cpp {
Preprocessor::Preprocessor(const String& filename, const StringView& program)
    : m_filename(filename)
    , m_program(program)
{
    GenericLexer program_lexer { m_program };
    for (;;) {
        if (program_lexer.is_eof())
            break;
        auto line = program_lexer.consume_until('\n');
        bool has_multiline = false;
        while (line.ends_with('\\') && !program_lexer.is_eof()) {
            auto continuation = program_lexer.consume_until('\n');
            line = StringView { line.characters_without_null_termination(), line.length() + continuation.length() + 1 };

            m_lines.append({});
            has_multiline = true;
        }

        if (has_multiline)
            m_lines.last() = line;
        else
            m_lines.append(line);
    }
}

Vector<Token> Preprocessor::process_and_lex()
{
    for (; m_line_index < m_lines.size(); ++m_line_index) {
        auto& line = m_lines[m_line_index];

        bool include_in_processed_text = false;
        if (line.starts_with("#")) {
            auto keyword = handle_preprocessor_line(line);
            if (m_options.keep_include_statements && keyword == "include")
                include_in_processed_text = true;
        } else if (m_state == State::Normal) {
            include_in_processed_text = true;
        }

        if (include_in_processed_text) {
            process_line(line);
        }
    }

    return m_tokens;
}

static void consume_whitespace(GenericLexer& lexer)
{
    auto ignore_line = [&] {
        for (;;) {
            if (lexer.consume_specific("\\\n"sv)) {
                lexer.ignore(2);
            } else {
                lexer.ignore_until('\n');
                break;
            }
        }
    };
    for (;;) {
        if (lexer.consume_specific("//"sv))
            ignore_line();
        else if (lexer.consume_specific("/*"sv))
            lexer.ignore_until("*/");
        else if (lexer.next_is("\\\n"sv))
            lexer.ignore(2);
        else if (lexer.is_eof() || !lexer.next_is(isspace))
            break;
        else
            lexer.ignore();
    }
}

Preprocessor::PreprocessorKeyword Preprocessor::handle_preprocessor_line(const StringView& line)
{
    GenericLexer lexer(line);

    consume_whitespace(lexer);
    lexer.consume_specific('#');
    consume_whitespace(lexer);
    auto keyword = lexer.consume_until(' ');
    if (keyword.is_empty() || keyword.is_null() || keyword.is_whitespace())
        return {};

    handle_preprocessor_keyword(keyword, lexer);
    return keyword;
}

void Preprocessor::handle_preprocessor_keyword(const StringView& keyword, GenericLexer& line_lexer)
{
    if (keyword == "include") {
        consume_whitespace(line_lexer);
        auto include_path = line_lexer.consume_all();
        m_included_paths.append(include_path);
        if (definitions_in_header_callback) {
            for (auto& def : definitions_in_header_callback(include_path))
                m_definitions.set(def.key, def.value);
        }
        return;
    }

    if (keyword == "else") {
        VERIFY(m_current_depth > 0);
        if (m_depths_of_not_taken_branches.contains_slow(m_current_depth - 1)) {
            m_depths_of_not_taken_branches.remove_all_matching([this](auto x) { return x == m_current_depth - 1; });
            m_state = State::Normal;
        }
        if (m_depths_of_taken_branches.contains_slow(m_current_depth - 1)) {
            m_state = State::SkipElseBranch;
        }
        return;
    }

    if (keyword == "endif") {
        VERIFY(m_current_depth > 0);
        --m_current_depth;
        if (m_depths_of_not_taken_branches.contains_slow(m_current_depth)) {
            m_depths_of_not_taken_branches.remove_all_matching([this](auto x) { return x == m_current_depth; });
        }
        if (m_depths_of_taken_branches.contains_slow(m_current_depth)) {
            m_depths_of_taken_branches.remove_all_matching([this](auto x) { return x == m_current_depth; });
        }
        m_state = State::Normal;
        return;
    }

    if (keyword == "define") {
        if (m_state == State::Normal) {
            auto key = line_lexer.consume_until(' ');
            consume_whitespace(line_lexer);

            DefinedValue value;
            value.filename = m_filename;
            value.line = m_line_index;

            auto string_value = line_lexer.consume_all();
            if (!string_value.is_empty())
                value.value = string_value;

            m_definitions.set(key, value);
        }
        return;
    }
    if (keyword == "undef") {
        if (m_state == State::Normal) {
            auto key = line_lexer.consume_until(' ');
            line_lexer.consume_all();
            m_definitions.remove(key);
        }
        return;
    }
    if (keyword == "ifdef") {
        ++m_current_depth;
        if (m_state == State::Normal) {
            auto key = line_lexer.consume_until(' ');
            if (m_definitions.contains(key)) {
                m_depths_of_taken_branches.append(m_current_depth - 1);
                return;
            } else {
                m_depths_of_not_taken_branches.append(m_current_depth - 1);
                m_state = State::SkipIfBranch;
                return;
            }
        }
        return;
    }
    if (keyword == "ifndef") {
        ++m_current_depth;
        if (m_state == State::Normal) {
            auto key = line_lexer.consume_until(' ');
            if (!m_definitions.contains(key)) {
                m_depths_of_taken_branches.append(m_current_depth - 1);
                return;
            } else {
                m_depths_of_not_taken_branches.append(m_current_depth - 1);
                m_state = State::SkipIfBranch;
                return;
            }
        }
        return;
    }
    if (keyword == "if") {
        ++m_current_depth;
        if (m_state == State::Normal) {
            
            m_depths_of_taken_branches.append(m_current_depth - 1);
        }
        return;
    }

    if (keyword == "elif") {
        VERIFY(m_current_depth > 0);

        if (m_depths_of_not_taken_branches.contains_slow(m_current_depth - 1) /* && should_take*/) {
            m_depths_of_not_taken_branches.remove_all_matching([this](auto x) { return x == m_current_depth - 1; });
            m_state = State::Normal;
        }
        if (m_depths_of_taken_branches.contains_slow(m_current_depth - 1)) {
            m_state = State::SkipElseBranch;
        }
        return;
    }
    if (keyword == "pragma") {
        line_lexer.consume_all();
        return;
    }

    if (!m_options.ignore_unsupported_keywords) {
        dbgln("Unsupported preprocessor keyword: {}", keyword);
        VERIFY_NOT_REACHED();
    }
}

void Preprocessor::process_line(StringView const& line)
{
    Lexer line_lexer { line, m_line_index };
    auto tokens = line_lexer.lex();

    for (auto& token : tokens) {
        if (token.type() == Token::Type::Whitespace)
            continue;
        if (token.type() == Token::Type::Identifier) {
            if (auto defined_value = m_definitions.find(token.text()); defined_value != m_definitions.end()) {
                do_substitution(token, defined_value->value);
                continue;
            }
        }
        m_tokens.append(token);
    }
}

void Preprocessor::do_substitution(Token const& replaced_token, DefinedValue const& defined_value)
{
    m_substitutions.append({ replaced_token, defined_value });

    if (defined_value.value.is_null())
        return;

    Lexer lexer(m_substitutions.last().defined_value.value);
    for (auto& token : lexer.lex()) {
        if (token.type() == Token::Type::Whitespace)
            continue;
        token.set_start(replaced_token.start());
        token.set_end(replaced_token.end());
        m_tokens.append(token);
    }
}

};
