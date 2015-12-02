/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AIDL_AST_JAVA_H_
#define AIDL_AST_JAVA_H_

#include <string>
#include <vector>
#include <set>
#include <stdarg.h>
#include <stdio.h>

using std::set;
using std::string;
using std::vector;

enum {
    PACKAGE_PRIVATE = 0x00000000,
    PUBLIC          = 0x00000001,
    PRIVATE         = 0x00000002,
    PROTECTED       = 0x00000003,
    SCOPE_MASK      = 0x00000003,

    STATIC          = 0x00000010,
    FINAL           = 0x00000020,
    ABSTRACT        = 0x00000040,

    OVERRIDE        = 0x00000100,

    ALL_MODIFIERS   = 0xffffffff
};

namespace android {
namespace aidl {
class CodeWriter;
}  // namespace aidl
}  // namespace android

namespace android {
namespace aidl {
namespace java {

class Type;

// Write the modifiers that are set in both mod and mask
void WriteModifiers(CodeWriter* to, int mod, int mask);

struct ClassElement
{
    ClassElement() = default;
    virtual ~ClassElement() = default;

    virtual void Write(CodeWriter* to) const = 0;
};

struct Expression
{
    virtual ~Expression() = default;
    virtual void Write(CodeWriter* to) const = 0;
};

struct LiteralExpression : public Expression
{
    string value;

    LiteralExpression(const string& value);
    virtual ~LiteralExpression() = default;
    void Write(CodeWriter* to) const override;
};

// TODO: also escape the contents.  not needed for now
struct StringLiteralExpression : public Expression
{
    string value;

    StringLiteralExpression(const string& value);
    virtual ~StringLiteralExpression() = default;
    void Write(CodeWriter* to) const override;
};

struct Variable : public Expression
{
    const Type* type = nullptr;
    string name;
    int dimension = 0;

    Variable() = default;
    Variable(const Type* type, const string& name);
    Variable(const Type* type, const string& name, int dimension);
    virtual ~Variable() = default;

    void WriteDeclaration(CodeWriter* to) const;
    void Write(CodeWriter* to) const;
};

struct FieldVariable : public Expression
{
    Expression* object;
    const Type* clazz;
    string name;

    FieldVariable(Expression* object, const string& name);
    FieldVariable(const Type* clazz, const string& name);
    virtual ~FieldVariable() = default;

    void Write(CodeWriter* to) const;
};

struct Field : public ClassElement
{
    string comment;
    int modifiers = 0;
    Variable *variable = nullptr;
    string value;

    Field() = default;
    Field(int modifiers, Variable* variable);
    virtual ~Field() = default;

    void Write(CodeWriter* to) const override;
};

struct Statement
{
    virtual ~Statement() = default;
    virtual void Write(CodeWriter* to) const = 0;
};

struct StatementBlock : public Statement
{
    vector<Statement*> statements;

    StatementBlock() = default;
    virtual ~StatementBlock() = default;
    void Write(CodeWriter* to) const override;

    void Add(Statement* statement);
    void Add(Expression* expression);
};

struct ExpressionStatement : public Statement
{
    Expression* expression;

    ExpressionStatement(Expression* expression);
    virtual ~ExpressionStatement() = default;
    void Write(CodeWriter* to) const override;
};

struct Assignment : public Expression
{
    Variable* lvalue;
    Expression* rvalue;
    const Type* cast;

    Assignment(Variable* lvalue, Expression* rvalue);
    Assignment(Variable* lvalue, Expression* rvalue, const Type* cast);
    virtual ~Assignment() = default;
    void Write(CodeWriter* to) const override;
};

struct MethodCall : public Expression
{
    Expression* obj = nullptr;
    const Type* clazz = nullptr;
    string name;
    vector<Expression*> arguments;
    vector<string> exceptions;

    MethodCall(const string& name);
    MethodCall(const string& name, int argc, ...);
    MethodCall(Expression* obj, const string& name);
    MethodCall(const Type* clazz, const string& name);
    MethodCall(Expression* obj, const string& name, int argc, ...);
    MethodCall(const Type* clazz, const string& name, int argc, ...);
    virtual ~MethodCall() = default;
    void Write(CodeWriter* to) const override;

private:
    void init(int n, va_list args);
};

struct Comparison : public Expression
{
    Expression* lvalue;
    string op;
    Expression* rvalue;

    Comparison(Expression* lvalue, const string& op, Expression* rvalue);
    virtual ~Comparison() = default;
    void Write(CodeWriter* to) const override;
};

struct NewExpression : public Expression
{
    const Type* type;
    vector<Expression*> arguments;

    NewExpression(const Type* type);
    NewExpression(const Type* type, int argc, ...);
    virtual ~NewExpression() = default;
    void Write(CodeWriter* to) const override;

private:
    void init(int n, va_list args);
};

struct NewArrayExpression : public Expression
{
    const Type* type;
    Expression* size;

    NewArrayExpression(const Type* type, Expression* size);
    virtual ~NewArrayExpression() = default;
    void Write(CodeWriter* to) const override;
};

struct Ternary : public Expression
{
    Expression* condition = nullptr;
    Expression* ifpart = nullptr;
    Expression* elsepart = nullptr;

    Ternary() = default;
    Ternary(Expression* condition, Expression* ifpart, Expression* elsepart);
    virtual ~Ternary() = default;
    void Write(CodeWriter* to) const override;
};

struct Cast : public Expression
{
    const Type* type = nullptr;
    Expression* expression = nullptr;

    Cast() = default;
    Cast(const Type* type, Expression* expression);
    virtual ~Cast() = default;
    void Write(CodeWriter* to) const override;
};

struct VariableDeclaration : public Statement
{
    Variable* lvalue = nullptr;
    const Type* cast = nullptr;
    Expression* rvalue = nullptr;

    VariableDeclaration(Variable* lvalue);
    VariableDeclaration(Variable* lvalue, Expression* rvalue, const Type* cast = NULL);
    virtual ~VariableDeclaration() = default;
    void Write(CodeWriter* to) const override;
};

struct IfStatement : public Statement
{
    Expression* expression = nullptr;
    StatementBlock* statements = new StatementBlock;
    IfStatement* elseif = nullptr;

    IfStatement() = default;
    virtual ~IfStatement() = default;
    void Write(CodeWriter* to) const override;
};

struct ReturnStatement : public Statement
{
    Expression* expression;

    ReturnStatement(Expression* expression);
    virtual ~ReturnStatement() = default;
    void Write(CodeWriter* to) const override;
};

struct TryStatement : public Statement
{
    StatementBlock* statements = new StatementBlock;

    TryStatement() = default;
    virtual ~TryStatement() = default;
    void Write(CodeWriter* to) const override;
};

struct CatchStatement : public Statement
{
    StatementBlock* statements;
    Variable* exception;

    CatchStatement(Variable* exception);
    virtual ~CatchStatement() = default;
    void Write(CodeWriter* to) const override;
};

struct FinallyStatement : public Statement
{
    StatementBlock* statements = new StatementBlock;

    FinallyStatement() = default;
    virtual ~FinallyStatement() = default;
    void Write(CodeWriter* to) const override;
};

struct Case
{
    vector<string> cases;
    StatementBlock* statements = new StatementBlock;

    Case() = default;
    Case(const string& c);
    virtual ~Case() = default;
    virtual void Write(CodeWriter* to) const;
};

struct SwitchStatement : public Statement
{
    Expression* expression;
    vector<Case*> cases;

    SwitchStatement(Expression* expression);
    virtual ~SwitchStatement() = default;
    void Write(CodeWriter* to) const override;
};

struct Break : public Statement
{
    Break() = default;
    virtual ~Break() = default;
    void Write(CodeWriter* to) const override;
};

struct Method : public ClassElement
{
    string comment;
    int modifiers = 0;
    const Type* returnType = nullptr;  // nullptr means constructor
    size_t returnTypeDimension = 0;
    string name;
    vector<Variable*> parameters;
    vector<const Type*> exceptions;
    StatementBlock* statements = nullptr;

    Method() = default;
    virtual ~Method() = default;

    void Write(CodeWriter* to) const override;
};

struct Constant : public ClassElement
{
    string name;
    int value;

    Constant() = default;
    virtual ~Constant() = default;

    void Write(CodeWriter* to) const override;
};

struct Class : public ClassElement
{
    enum {
        CLASS,
        INTERFACE
    };

    string comment;
    int modifiers = 0;
    int what = CLASS;               // CLASS or INTERFACE
    const Type* type = nullptr;
    const Type* extends = nullptr;
    vector<const Type*> interfaces;
    vector<ClassElement*> elements;

    Class() = default;
    virtual ~Class() = default;

    void Write(CodeWriter* to) const override;
};

struct Document
{
    string comment;
    string package;
    string originalSrc;
    vector<Class*> classes;

    Document() = default;
    virtual ~Document() = default;

    virtual void Write(CodeWriter* to) const;
};

}  // namespace java
}  // namespace aidl
}  // namespace android

#endif // AIDL_AST_JAVA_H_
