# ai_usage.md

 AI Tool used: Claude (claude.ai)
 
The Generated functions

Function parse_condition
Prompt: "Generate a C function `int parse_condition(const char *input, char *field, char *op, char *value)` that splits a `field:op:value` string into three parts."

Code:
int parse_condition(const char *input, char *field, char *op, char *value) {

    const char *colon1 = strchr(input, ':');
    if (!colon1) return 0;

    size_t flen = colon1 - input;    
    strcpy(field, input, flen);
    field[flen] = '\0';            

    const char *colon2 = strchr(colon1 + 1, ':');
    if (!colon2) return 0;

    size_t olen = colon2 - colon1 - 1;
    strcpy(op, colon1 + 1, olen);
    op[olen] = '\0';

    strcpy(value, colon2 + 1, 63);
    value[63] = '\0';

    return 1;
}

Changes made:
- Replaced `strcpy` with `strncpy`
- Added operator validation



Function: match_condition
Prompt: "Generate a C function `int match_condition(Report *r, const char *field, const char *op, const char *value)` that returns 1 if the report matches the condition."

Code:
int match_condition(Report *r, const char *field, const char *op, const char *value) {

    if (strcmp(field, "severity") == 0) {
        int rval = r->severity;
        int cval = atoi(value);  

        if (strcmp(op, "==") == 0) return rval == cval;
        if (strcmp(op, "!=") == 0) return rval != cval;
        if (strcmp(op, "<")  == 0) return rval <  cval;
        if (strcmp(op, "<=") == 0) return rval <= cval;
        if (strcmp(op, ">")  == 0) return rval >  cval;
        if (strcmp(op, ">=") == 0) return rval >= cval;
    }

    if (strcmp(field, "timestamp") == 0) {
        time_t rval = r->timestamp;
        time_t cval = (time_t)atol(value);
       
    }

    // --- champs chaînes ---
    if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
      
    }
    
}

Changes:
- `timestamp` field was missing — added with `atol()`
- `category` and `inspector` only handled `==` — added remaining operators
- Added `atoi()` for `severity` type conversion

## What I learned
The AI generates a functional skeleton but misses edge cases.
You have to check after IA to see if no mistakes were made with a functions and also verifies if the function works with the rest of your code.
