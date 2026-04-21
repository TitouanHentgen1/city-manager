#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[50];
    float score;
    char department[30];
} Report;

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (r == NULL || field == NULL || op == NULL || value == NULL) return 0;

    // --- Integer Field: id ---
    if (strcmp(field, "id") == 0) {
        int v = atoi(value);
        if (strcmp(op, "==") == 0) return r->id == v;
        if (strcmp(op, "!=") == 0) return r->id != v;
        if (strcmp(op, "<") == 0)  return r->id < v;
        if (strcmp(op, ">") == 0)  return r->id > v;
    }
    // --- String Field: name ---
    else if (strcmp(field, "name") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->name, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->name, value) != 0;
        if (strcmp(op, "like") == 0) return strstr(r->name, value) != NULL;
    }
    // --- Float Field: score ---
    else if (strcmp(field, "score") == 0) {
        float v = atof(value);
        if (strcmp(op, "==") == 0) return r->score == v;
        if (strcmp(op, "!=") == 0) return r->score != v;
        if (strcmp(op, "<") == 0)  return r->score < v;
        if (strcmp(op, ">") == 0)  return r->score > v;
    }
    // --- String Field: department ---
    else if (strcmp(field, "department") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->department, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->department, value) != 0;
        if (strcmp(op, "like") == 0) return strstr(r->department, value) != NULL;
    }

    return 0; // No match found or unsupported field/operator
}
int main() {
    Report r1 = {101, "Alice", 95.5, "Engineering"};

    // Test: name == "Alice"
    if (match_condition(&r1, "name", "==", "Alice")) {
        printf("Matched Alice\n");
    }

    // Test: score > 90
    if (match_condition(&r1, "score", ">", "90.0")) {
        printf("High score\n");
    }

    return 0;
}
