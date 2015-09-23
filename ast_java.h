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
class Type;

// Write the modifiers that are set in both mod and mask
void WriteModifiers(CodeWriter* to, int mod, int mask);

struct ClassElement
{
    ClassElement();
    virtual ~ClassElement();

    virtual void GatherTypes(set<const Type*>* types) const = 0;
    virtual void Write(CodeWriter* to) const = 0;
};

struct Expression
{
    virtual ~Expression();
    virtual void Write(CodeWriter* to) const = 0;
};

struct LiteralExpression : public Expression
{
    string value;

    LiteralExpression(const string& value);
    virtual ~LiteralExpression();
    virtual void Write(CodeWriter* to) const;
};

// TODO: also escape the contents.  not needed for now
struct StringLiteralExpression : public Expression
{
    string value;

    StringLiteralExpression(const string& value);
    virtual ~StringLiteralExpression();
    virtual void Write(CodeWriter* to) const;
};

struct Variable : public Expression
{
    const Type* type;
    string name;
    int dimension;

    Variable();
    Variable(const Type* type, const string& name);
    Variable(const Type* type, const string& name, int dimension);
    virtual ~Variable();

    virtual void GatherTypes(set<const Type*>* types) const;
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
    virtual ~FieldVariable();

    void Write(CodeWriter* to) const;
};

struct Field : public ClassElement
{
    string comment;
    int modifiers;
    Variable *variable;
    string value;

    Field();
    Field(int modifiers, Variable* variable);
    virtual ~Field();

    virtual void GatherTypes(set<const Type*>* types) const;
    virtual void Write(CodeWriter* to) const;
};

struct Statement
{
    virtual ~Statement();
    virtual void Write(CodeWriter* to) const = 0;
};

struct StatementBlock : public Statement
{
    vector<Statement*> statements;

    StatementBlock();
    virtual ~StatementBlock();
    virtual void Write(CodeWriter* to) const;

    void Add(Statement* statement);
    void Add(Expression* expression);
};

struct ExpressionStatement : public Statement
{
    Expression* expression;

    ExpressionStatement(Expression* expression);
    virtual ~ExpressionStatement();
    virtual void Write(CodeWriter* to) const;
};

struct Assignment : public Expression
{
    Variable* lvalue;
    Expression* rvalue;
    const Type* cast;

    Assignment(Variable* lvalue, Expression* rvalue);
    Assignment(Variable* lvalue, Expression* rvalue, const Type* cast);
    virtual ~Assignment();
    virtual void Write(CodeWriter* to) const;
};

struct MethodCall : public Expression
{
    Expression* obj;
    const Type* clazz;
    string name;
    vector<Expression*> arguments;
    vector<string> exceptions;

    MethodCall(const string& name);
    MethodCall(const string& name, int argc, ...);
    MethodCall(Expression* obj, const string& name);
    MethodCall(const Type* clazz, const string& name);
    MethodCall(Expression* obj, const string& name, int argc, ...);
    MethodCall(const Type* clazz, const string& name, int argc, ...);
    virtual ~MethodCall();
    virtual void Write(CodeWriter* to) const;

private:
    void init(int n, va_list args);
};

struct Comparison : public Expression
{
    Expression* lvalue;
    string op;
    Expression* rvalue;

    Comparison(Expression* lvalue, const string& op, Expression* rvalue);
    virtual ~Comparison();
    virtual void Write(CodeWriter* to) const;
};

struct NewExpression : public Expression
{
    const Type* type;
    vector<Expression*> arguments;

    NewExpression(const Type* type);
    NewExpression(const Type* type, int argc, ...);
    virtual ~NewExpression();
    virtual void Write(CodeWriter* to) const;

private:
    void init(int n, va_list args);
};

struct NewArrayExpression : public Expression
{
    const Type* type;
    Expression* size;

    NewArrayExpression(const Type* type, Expression* size);
    virtual ~NewArrayExpression();
    virtual void Write(CodeWriter* to) const;
};

struct Ternary : public Expression
{
    Expression* condition;
    Expression* ifpart;
    Expression* elsepart;

    Ternary();
    Ternary(Expression* condition, Expression* ifpart, Expression* elsepart);
    virtual ~Ternary();
    virtual void Write(CodeWriter* to) const;
};

struct Cast : public Expression
{
    const Type* type;
    Expression* expression;

    Cast();
    Cast(const Type* type, Expression* expression);
    virtual ~Cast();
    virtual void Write(CodeWriter* to) const;
};

struct VariableDeclaration : public Statement
{
    Variable* lvalue;
    const Type* cast;
    Expression* rvalue;

    VariableDeclaration(Variable* lvalue);
    VariableDeclaration(Variable* lvalue, Expression* rvalue, const Type* cast = NULL);
    virtual ~VariableDeclaration();
    virtual void Write(CodeWriter* to) const;
};

struct IfStatement : public Statement
{
    Expression* expression;
    StatementBlock* statements;
    IfStatement* elseif;

    IfStatement();
    virtual ~IfStatement();
    virtual void Write(CodeWriter* to) const;
};

struct ReturnStatement : public Statement
{
    Expression* expression;

    ReturnStatement(Expression* expression);
    virtual ~ReturnStatement();
    virtual void Write(CodeWriter* to) const;
};

struct TryStatement : public Statement
{
    StatementBlock* statements;

    TryStatement();
    virtual ~TryStatement();
    virtual void Write(CodeWriter* to) const;
};

struct CatchStatement : public Statement
{
    StatementBlock* statements;
    Variable* exception;

    CatchStatement(Variable* exception);
    virtual ~CatchStatement();
    virtual void Write(CodeWriter* to) const;
};

struct FinallyStatement : public Statement
{
    StatementBlock* statements;

    FinallyStatement();
    virtual ~FinallyStatement();
    virtual void Write(CodeWriter* to) const;
};

struct Case
{
    vector<string> cases;
    StatementBlock* statements;

    Case();
    Case(const string& c);
    virtual ~Case();
    virtual void Write(CodeWriter* to) const;
};

struct SwitchStatement : public Statement
{
    Expression* expression;
    vector<Case*> cases;

    SwitchStatement(Expression* expression);
    virtual ~SwitchStatement();
    virtual void Write(CodeWriter* to) const;
};

struct Break : public Statement
{
    Break();
    virtual ~Break();
    virtual void Write(CodeWriter* to) const;
};

struct Method : public ClassElement
{
    string comment;
    int modifiers;
    const Type* returnType;
    size_t returnTypeDimension;
    string name;
    vector<Variable*> parameters;
    vector<const Type*> exceptions;
    StatementBlock* statements;

    Method();
    virtual ~Method();

    virtual void GatherTypes(set<const Type*>* types) const;
    virtual void Write(CodeWriter* to) const;
};

struct Class : public ClassElement
{
    enum {
        CLASS,
        INTERFACE
    };

    string comment;
    int modifiers;
    int what;               // CLASS or INTERFACE
    const Type* type;
    const Type* extends;
    vector<const Type*> interfaces;
    vector<ClassElement*> elements;

    Class();
    virtual ~Class();

    virtual void GatherTypes(set<const Type*>* types) const;
    virtual void Write(CodeWriter* to) const;
};

struct Document
{
    string comment;
    string package;
    string originalSrc;
    set<const Type*> imports;
    vector<Class*> classes;

    Document();
    virtual ~Document();

    virtual void Write(CodeWriter* to) const;
};

}  // namespace aidl
}  // namespace android

#endif // AIDL_AST_JAVA_H_
