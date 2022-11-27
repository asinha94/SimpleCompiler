#include <string>


static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;


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
        
        NumVal = strtod(num.c_str()), 0;
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
