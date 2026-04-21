#include <stdio.h>
#include <string.h>

/**
 * Parses a "field:operator:value" string.
 * Returns 1 on success, 0 on failure.
 */
int parse_condition(const char *input, char *field, char *op, char *value) {
    if (input == NULL || field == NULL || op == NULL || value == NULL) {
        return 0;
    }

    // Find the first colon (end of field)
    const char *first_colon = strchr(input, ':');
    if (first_colon == NULL) return 0;

    // Find the second colon (end of operator)
    const char *second_colon = strchr(first_colon + 1, ':');
    if (second_colon == NULL) return 0;

    // Calculate lengths
    size_t field_len = first_colon - input;
    size_t op_len = second_colon - (first_colon + 1);

    // Copy parts into output buffers
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    strncpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    strcpy(value, second_colon + 1);

    return 1; // Success
}

// Example usage
int main() {
    char input[] = "age:>:25";
    char f[32], o[8], v[32];

    if (parse_condition(input, f, o, v)) {
        printf("Field: %s, Op: %s, Value: %s\n", f, o, v);
    }
    return 0;
}
