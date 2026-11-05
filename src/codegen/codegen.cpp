#include "../include.h"

extern "C" Value *db_open(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = Builder->CreateExtractValue(ArgsV[0], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[0], {1}), intTy, true);
    return callret("db_open_C", {scope_struct, ptr, size});
}

extern "C" Value *db_exec(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = Builder->CreateExtractValue(ArgsV[1], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[1], {1}), intTy, true);
    return callret("db_exec_C", {scope_struct, ArgsV[0], ptr, size});
}

extern "C" Value *db_prepare(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = Builder->CreateExtractValue(ArgsV[1], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[1], {1}), intTy, true);
    return callret("db_prepare_C", {scope_struct, ArgsV[0], ptr, size});
}

extern "C" Value *db_bind_text(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = Builder->CreateExtractValue(ArgsV[2], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[2], {1}), intTy, true);
    return callret("db_bind_text_C", {scope_struct, ArgsV[0], ArgsV[1], ptr, size});
}

extern "C" Value *db_column_text(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *size = callret("db_column_bytes_C", {scope_struct, ArgsV[0], ArgsV[1]});
    Value *ptr = callret("db_column_text_C", {scope_struct, ArgsV[0], ArgsV[1]});
    Value *text = UndefValue::get(struct_types["DT_str"]);
    text = Builder->CreateInsertValue(text, ptr, {0});
    return Builder->CreateInsertValue(text, Builder->CreateIntCast(size, struct_types["DT_str"]->getElementType(1), false), {1});
}

extern "C" Value *db_error(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = callret("db_error_C", {scope_struct});
    Value *text = UndefValue::get(struct_types["DT_str"]);
    text = Builder->CreateInsertValue(text, ptr, {0});
    Value *size = Builder->CreateIntCast(callret("strlen", {ptr}), struct_types["DT_str"]->getElementType(1), false);
    return Builder->CreateInsertValue(text, size, {1});
}

extern "C" Value *db_sql(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = Builder->CreateExtractValue(ArgsV[1], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[1], {1}), intTy, true);
    return callret("db_sql_C", {scope_struct, ArgsV[0], ptr, size});
}

extern "C" Value *db_set_error(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = Builder->CreateExtractValue(ArgsV[0], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[0], {1}), intTy, true);
    return callret("db_set_error_C", {scope_struct, ptr, size});
}

extern "C" Value *db_quote_ident(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *input = Builder->CreateExtractValue(ArgsV[0], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[0], {1}), intTy, true);
    Value *ptr = callret("db_quote_ident_C", {scope_struct, input, size});
    Value *text = UndefValue::get(struct_types["DT_str"]);
    text = Builder->CreateInsertValue(text, ptr, {0});
    Value *len = Builder->CreateIntCast(callret("strlen", {ptr}), struct_types["DT_str"]->getElementType(1), false);
    return Builder->CreateInsertValue(text, len, {1});
}

extern "C" Value *db_column_index(Parser_Struct *parser_struct, Function *TheFunction,
    std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
    Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *ptr = Builder->CreateExtractValue(ArgsV[1], {0});
    Value *size = Builder->CreateIntCast(Builder->CreateExtractValue(ArgsV[1], {1}), intTy, true);
    return callret("db_column_index_C", {scope_struct, ArgsV[0], ptr, size});
}
