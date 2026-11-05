#include "include.h"
#include <sqlite3.h>
#include <cstring>
#include <iostream>
#include <string>

namespace {
thread_local std::string last_error;
int record_error(sqlite3 *db, int rc) {
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE)
        last_error = db ? sqlite3_errmsg(db) : sqlite3_errstr(rc);
    return rc;
}
std::string checked_text(char *data, int size) {
    return data && size > 0 ? std::string(data, size) : std::string();
}

std::string display_text(const char *data, int size) {
    std::string result;
    for (int i = 0; i < size; ++i) {
        switch (data[i]) {
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\0': result += "\\0"; break;
            case '|': result += "\\|"; break;
            case '\\': result += "\\\\"; break;
            default: result += data[i]; break;
        }
    }
    return result;
}

std::string display_column(sqlite3_stmt *stmt, int column) {
    int type = sqlite3_column_type(stmt, column);
    if (type == SQLITE_NULL) return "NULL";
    if (type == SQLITE_BLOB) {
        const unsigned char *data = static_cast<const unsigned char *>(sqlite3_column_blob(stmt, column));
        int size = sqlite3_column_bytes(stmt, column);
        const char *hex = "0123456789ABCDEF";
        std::string result = "X'";
        for (int i = 0; i < size; ++i) {
            result += hex[data[i] >> 4];
            result += hex[data[i] & 15];
        }
        return result + "'";
    }
    const unsigned char *text = sqlite3_column_text(stmt, column);
    if (!text) return "";
    return display_text(reinterpret_cast<const char *>(text), sqlite3_column_bytes(stmt, column));
}

int execute_sql(sqlite3 *db, const std::string &sql) {
    if (!db) return record_error(nullptr, SQLITE_MISUSE);
    if (sql.find('\0') != std::string::npos) {
        last_error = "SQL contains a NUL byte";
        return SQLITE_MISUSE;
    }
    const char *cursor = sql.c_str();
    const char *end = cursor + sql.size();
    while (cursor < end) {
        sqlite3_stmt *stmt = nullptr;
        const char *tail = nullptr;
        int rc = sqlite3_prepare_v2(db, cursor, static_cast<int>(end - cursor), &stmt, &tail);
        if (rc != SQLITE_OK) {
            record_error(db, rc);
            if (stmt) sqlite3_finalize(stmt);
            return rc;
        }
        if (!tail || tail <= cursor) {
            if (stmt) sqlite3_finalize(stmt);
            last_error = "SQLite did not advance while parsing SQL";
            return SQLITE_ERROR;
        }
        cursor = tail;

        if (!stmt) continue;
        int columns = sqlite3_column_count(stmt);
        if (columns > 0) {
            for (int i = 0; i < columns; ++i) {
                if (i) std::cout << " | ";
                const char *name = sqlite3_column_name(stmt, i);
                std::cout << (name ? display_text(name, std::strlen(name)) : "");
            }
            std::cout << '\n';
        }
        sqlite3_int64 rows = 0;
        sqlite3_int64 before = sqlite3_total_changes64(db);
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            for (int i = 0; i < columns; ++i) {
                if (i) std::cout << " | ";
                std::cout << display_column(stmt, i);
            }
            std::cout << '\n';
            ++rows;
        }
        if (rc != SQLITE_DONE) {
            record_error(db, rc);
            sqlite3_finalize(stmt);
            return rc;
        }
        sqlite3_int64 changed = sqlite3_total_changes64(db) - before;
        rc = sqlite3_finalize(stmt);
        if (rc != SQLITE_OK) return record_error(db, rc);
        if (columns > 0) std::cout << rows << " linha(s) retornada(s).\n";
        else std::cout << "OK. " << changed << " linha(s) alterada(s).\n";
    }
    return SQLITE_OK;
}

std::string trim_line(const std::string &text) {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

void console_help() {
    std::cout << "Digite SQL e termine com ;. Comandos podem ocupar varias linhas.\n"
              << ".tabelas  listar tabelas\n"
              << ".cancelar limpar o comando pendente\n"
              << ".ajuda    mostrar esta ajuda\n"
              << ".sair     fechar o console\n";
}
}

extern "C" void *db_open_C(Scope_Struct *ctx, char *path, int size) {
    std::string filename = checked_text(path, size);
    sqlite3 *db = nullptr;
    if (filename.find('\0') != std::string::npos) {
        last_error = "Database path contains a NUL byte";
        return nullptr;
    }
    int rc = sqlite3_open(filename.c_str(), &db);
    if (rc != SQLITE_OK) {
        record_error(db, rc);
        if (db) sqlite3_close(db);
        return nullptr;
    }
    sqlite3_busy_timeout(db, 5000);
    last_error.clear();
    return db;
}

extern "C" int db_valid(Scope_Struct *ctx, void *handle) {
    return handle != nullptr;
}

extern "C" int db_close(Scope_Struct *ctx, void *handle) {
    sqlite3 *db = static_cast<sqlite3 *>(handle);
    if (!db) return SQLITE_OK;
    int rc = sqlite3_close(db);

    if (rc != SQLITE_OK) record_error(db, rc);
    return rc;
}

extern "C" int db_exec_C(Scope_Struct *ctx, void *handle, char *sql, int size) {
    sqlite3 *db = static_cast<sqlite3 *>(handle);
    if (!db) return record_error(nullptr, SQLITE_MISUSE);
    std::string query = checked_text(sql, size);
    if (query.find('\0') != std::string::npos) {
        last_error = "SQL contains a NUL byte";
        return SQLITE_MISUSE;
    }
    return record_error(db, sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr));
}

extern "C" void *db_prepare_C(Scope_Struct *ctx, void *handle, char *sql, int size) {
    sqlite3 *db = static_cast<sqlite3 *>(handle);
    if (!db) { record_error(nullptr, SQLITE_MISUSE); return nullptr; }
    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, size, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        record_error(db, rc);
        if (stmt) sqlite3_finalize(stmt);
        return nullptr;
    }
    if (!stmt) last_error = "SQL does not contain a statement";
    return stmt;
}

extern "C" int db_bind_int(Scope_Struct *ctx, void *handle, int index, int value) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return record_error(nullptr, SQLITE_MISUSE);
    return record_error(sqlite3_db_handle(stmt), sqlite3_bind_int(stmt, index, value));
}

extern "C" int db_bind_float(Scope_Struct *ctx, void *handle, int index, float value) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return record_error(nullptr, SQLITE_MISUSE);
    return record_error(sqlite3_db_handle(stmt), sqlite3_bind_double(stmt, index, static_cast<double>(value)));
}

extern "C" int db_bind_null(Scope_Struct *ctx, void *handle, int index) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return record_error(nullptr, SQLITE_MISUSE);
    return record_error(sqlite3_db_handle(stmt), sqlite3_bind_null(stmt, index));
}

extern "C" int db_bind_text_C(Scope_Struct *ctx, void *handle, int index, char *text, int size) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return record_error(nullptr, SQLITE_MISUSE);
    return record_error(sqlite3_db_handle(stmt), sqlite3_bind_text(stmt, index, text, size, SQLITE_TRANSIENT));
}

extern "C" int db_step(Scope_Struct *ctx, void *handle) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return record_error(nullptr, SQLITE_MISUSE);
    return record_error(sqlite3_db_handle(stmt), sqlite3_step(stmt));
}

extern "C" int db_reset(Scope_Struct *ctx, void *handle) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return record_error(nullptr, SQLITE_MISUSE);
    return record_error(sqlite3_db_handle(stmt), sqlite3_reset(stmt));
}

extern "C" int db_finalize(Scope_Struct *ctx, void *handle) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return SQLITE_OK;
    sqlite3 *db = sqlite3_db_handle(stmt);
    return record_error(db, sqlite3_finalize(stmt));
}

extern "C" int db_column_int(Scope_Struct *ctx, void *handle, int column) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    return stmt ? sqlite3_column_int(stmt, column) : 0;
}

extern "C" float db_column_float(Scope_Struct *ctx, void *handle, int column) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    return stmt ? static_cast<float>(sqlite3_column_double(stmt, column)) : 0.0f;
}

extern "C" int db_column_is_null(Scope_Struct *ctx, void *handle, int column) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    return stmt ? sqlite3_column_type(stmt, column) == SQLITE_NULL : 1;
}

extern "C" int db_column_bytes_C(Scope_Struct *ctx, void *handle, int column) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) return 0;
    sqlite3_column_text(stmt, column);
    return sqlite3_column_bytes(stmt, column);
}

extern "C" char *db_column_text_C(Scope_Struct *ctx, void *handle, int column) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    const unsigned char *text = stmt ? sqlite3_column_text(stmt, column) : nullptr;
    int size = text ? sqlite3_column_bytes(stmt, column) : 0;
    char *copy = static_cast<char *>(allocate_pool(ctx, size + 1, data_name_to_type()["str"]));
    if (size) std::memcpy(copy, text, size);
    copy[size] = '\0';
    return copy;
}

extern "C" char *db_error_C(Scope_Struct *ctx) {
    return CopyString(ctx, const_cast<char *>(last_error.c_str()));
}

extern "C" int db_set_error_C(Scope_Struct *ctx, char *text, int size) {
    last_error = checked_text(text, size);
    return SQLITE_MISUSE;
}

extern "C" void *db_null_handle(Scope_Struct *ctx) {
    return nullptr;
}

extern "C" int db_changes(Scope_Struct *ctx, void *handle) {
    sqlite3 *db = static_cast<sqlite3 *>(handle);
    if (!db) { record_error(nullptr, SQLITE_MISUSE); return -1; }
    return sqlite3_changes(db);
}

extern "C" char *db_quote_ident_C(Scope_Struct *ctx, char *text, int size) {
    std::string name = checked_text(text, size);
    std::string quoted;
    if (name.empty() || name.find('\0') != std::string::npos) {
        last_error = "Nome de tabela ou coluna vazio ou com byte NUL";
        return CopyString(ctx, const_cast<char *>(""));
    }
    size_t begin = 0;
    while (begin <= name.size()) {
        size_t dot = name.find('.', begin);
        size_t end = dot == std::string::npos ? name.size() : dot;
        if (end == begin) {
            last_error = "Qualificador SQL vazio";
            return CopyString(ctx, const_cast<char *>(""));
        }
        if (!quoted.empty()) quoted += '.';
        quoted += '"';
        for (size_t i = begin; i < end; ++i) {
            if (name[i] == '"') quoted += '"';
            quoted += name[i];
        }
        quoted += '"';
        if (dot == std::string::npos) break;
        begin = dot + 1;
    }
    return CopyString(ctx, const_cast<char *>(quoted.c_str()));
}

extern "C" int db_column_index_C(Scope_Struct *ctx, void *handle, char *text, int size) {
    sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(handle);
    if (!stmt) { record_error(nullptr, SQLITE_MISUSE); return -1; }
    std::string name = checked_text(text, size);
    int index = -1;
    for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
        const char *candidate = sqlite3_column_name(stmt, i);
        if (candidate && name == candidate) {
            if (index >= 0) {
                last_error = "Coluna ambigua; selecione um alias: " + name;
                return -1;
            }
            index = i;
        }
    }
    if (index < 0) last_error = "Coluna inexistente no resultado: " + name;
    return index;
}

extern "C" int db_sql_C(Scope_Struct *ctx, void *handle, char *sql, int size) {
    int rc = execute_sql(static_cast<sqlite3 *>(handle), checked_text(sql, size));
    std::cout << std::flush;
    return rc;
}

extern "C" int db_console(Scope_Struct *ctx, void *handle) {
    sqlite3 *db = static_cast<sqlite3 *>(handle);
    if (!db) return record_error(nullptr, SQLITE_MISUSE);
    std::cout << "Console SQL - banco: " << sqlite3_db_filename(db, "main") << '\n';
    console_help();
    std::string pending;
    std::string line;
    while (true) {
        std::cout << (pending.empty() ? "sql> " : "...> ") << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            if (!trim_line(pending).empty()) {
                last_error = "SQL incompleto: termine o comando com ; antes de encerrar";
                return SQLITE_MISUSE;
            }
            break;
        }
        std::string command = trim_line(line);
        if (command == ".sair" || command == ".exit" || command == ".quit") break;
        if (command == ".cancelar") { pending.clear(); continue; }
        if (command == ".ajuda") { console_help(); continue; }
        if (pending.empty() && command.empty()) continue;
        if (pending.empty() && command == ".tabelas") {
            int rc = execute_sql(db, "SELECT name AS tabela FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name;");
            if (rc != SQLITE_OK) std::cout << "Erro SQL: " << last_error << '\n';
            continue;
        }
        pending += line;
        pending += '\n';
        int complete = sqlite3_complete(pending.c_str());
        if (complete == 0) continue;
        if (complete != 1) {
            record_error(db, complete);
            std::cout << "Erro SQL: " << last_error << '\n';
            pending.clear();
            continue;
        }
        int rc = execute_sql(db, pending);
        if (rc != SQLITE_OK) std::cout << "Erro SQL: " << last_error << '\n';
        pending.clear();
    }
    std::cout << "Console encerrado.\n" << std::flush;
    return SQLITE_OK;
}
