#include <string>
#include <map>
#include <vector>
#include "ast.h"


static int CurTok;
static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;
// Binary Parsing
static std::map<char, int> BinopPrecedence;

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int, std::unique_ptr<ExprAST>);


enum TOKEN {
    T_EOF = -1,
    T_DEF = -2,
    T_EXTERN = -3,
    T_IDENTIFIER = -4,
    T_NUMBER = -5
};


int GetToken()
{
    static char last_char;
    // Find the first non-space char
    while (isspace(last_char = getchar()));

    if (isalpha(last_char)) {
        // Put the whole string into identifier
        IdentifierStr = last_char;
        while (isalnum(last_char = getchar()))
            IdentifierStr += last_char;

        if (IdentifierStr == "def")
            return T_DEF;

        if (IdentifierStr == "extern")
            return T_EXTERN;

        return T_IDENTIFIER;
    }

    if (isdigit(last_char)) {
        std::string num;

        do {
            num += last_char;
            last_char = getchar();
        } while ( isdigit(last_char) || last_char == '.');
        
        NumVal = strtod(num.c_str(), 0);
        return T_NUMBER; 
    }

    if (last_char == '#') {
    // Comment until end of line.
    do
        last_char = getchar();
    while (last_char != EOF && last_char != '\n' && last_char != '\r');

    if (last_char != EOF)
        return GetToken();
    }

    if (last_char == EOF)
        return T_EOF;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = last_char;
    last_char = getchar();
    return last_char;

}

// Helper Functions
static int GetNextToken() {
    return CurTok = GetToken();
}

std::unique_ptr<ExprAST> LogError(const char * str) {
    fprintf(stderr, "LogError: %s\n", str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char * str) {
    LogError(str);
    return nullptr;
}


// Parsers
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto result = std::make_unique<NumberExprAST>(NumVal);
    GetNextToken();
    return std::move(result);
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
    GetNextToken(); // swallow the (
    auto v = ParseExpression(); // This can be recursive
    if (!v)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected (");

    GetNextToken(); // eat the  )
    return v;
}

// Handles identifier expressions i.e
//  a
//  a = b
//  a = f()
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    // Eat identifier? I dont know how this works
    GetNextToken();

    // Check if just `identifer` on its own
    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(IdName);

    GetNextToken(); // eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    
    if (CurTok != ')') { // We have arguments to process
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if(CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");

            GetNextToken();
        }
    }

    GetNextToken();
    return std::make_unique<CallExprAST>(IdName, std::move(Args)); 
}

// Parses expression e.g.
//  a = b
//  a = 5
//  a = f()
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("Unknown token while when expecting an expression");
        case T_IDENTIFIER:
            return ParseIdentifierExpr();
        case T_NUMBER:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

static int GetTokPrecendence() {
    if (!isascii(CurTok))
        return -1;

    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

// a + b + (c+d)*e*f + g

// a + b * c + d
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprePrec, std::unique_ptr<ExprAST> LHS) {
    while(true) {
        int TokPrec = GetTokPrecendence();

        if (TokPrec < ExprePrec)
            return LHS;

        int binOp = CurTok;
        GetNextToken();

        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        int NextPrec = GetTokPrecendence();
        if (TokPrec < NextPrec) {
            int NextPrec = GetTokPrecendence();
            if (TokPrec < NextPrec) {
                RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
                if (!RHS)
                    return nullptr;
            }
        }

        LHS = std::make_unique<BinaryExprAST>(binOp, std::move(LHS), std::move(RHS));
    }
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != T_IDENTIFIER)
        return LogErrorP("Expected function name in prototype");

    std::string fnName = IdentifierStr;
    GetNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

        // Read the list of argument names.
    std::vector<std::string> ArgNames;
    while (GetNextToken() == T_IDENTIFIER)
        ArgNames.push_back(IdentifierStr);
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success.
    GetNextToken();  // eat ')'.

    return std::make_unique<PrototypeAST>(fnName, std::move(ArgNames));
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
    GetNextToken();  // eat def.
    auto Proto = ParsePrototype();
    if (!Proto) return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
    GetNextToken();  // eat extern.
    return ParsePrototype();
}


/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

static void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        GetNextToken();
    }
}

static void HandleExtern() {
    if (ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // Skip token for error recovery.
        GetNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // Skip token for error recovery.
        GetNextToken();
    }
}


static void MainLoop() {
    while(1) {
        fprintf(stderr, "read> ");
        switch(CurTok) {
            case T_EOF:
                return;
            case ';': // ignore top-level semicolons.
                GetNextToken();
                break;
            case T_DEF:
                HandleDefinition();
                break;
            case T_EXTERN:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}

int main() {
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    fprintf(stderr, "ready> ");
    GetNextToken();

    MainLoop();


    return 0;
}