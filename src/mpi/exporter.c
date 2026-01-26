#include "exporter.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Simple strdup replacement (returns heap-allocated copy; caller must free). */
static char* my_strdup(const char *s) {
    if (s == NULL) s = "";
    size_t n = strlen(s) + 1;
    char *r = (char*)malloc(n);
    if (r) memcpy(r, s, n);
    return r;
}

/* Escape a CSV field according to RFC-style rules:
   - If the field contains a comma, quote, CR or LF, wrap the whole field in quotes.
   - Double any internal quote characters.
   Returns a newly allocated string which the caller must free.
*/
static char* escape_csv(const char *s) {
    if (s == NULL) return my_strdup("");

    bool need_quotes = false;
    size_t len = 0;
    size_t quote_count = 0;
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        unsigned char c = *p;
        if (c == '"' ) quote_count++;
        if (c == ',' || c == '"' || c == '\n' || c == '\r') need_quotes = true;
        len++;
    }

    size_t newlen = len + quote_count; /* each internal " becomes two chars */
    if (need_quotes) newlen += 2;      /* surrounding quotes */
    newlen += 1;                       /* NUL terminator */

    char *out = (char*)malloc(newlen);
    if (!out) return NULL;

    char *q = out;
    if (need_quotes) *q++ = '"';
    for (const char *p = s; *p; ++p) {
        if (*p == '"') {
            *q++ = '"';
            *q++ = '"';
        } else {
            *q++ = *p;
        }
    }
    if (need_quotes) *q++ = '"';
    *q = '\0';
    return out;
}

static char* format_double(double d) {
    char buf[64];
    /* print with fixed precision then strip trailing zeros and possible dot */
    snprintf(buf, sizeof(buf), "%.6f", d);
    size_t n = strlen(buf);
    if (strchr(buf, '.')) {
        while (n > 0 && buf[n-1] == '0') { buf[--n] = '\0'; }
        if (n > 0 && buf[n-1] == '.') { buf[--n] = '\0'; }
    }
    return my_strdup(buf);
}

void appendToCSV(Header* h, Values* v, char* path) {
    if (h == NULL && v == NULL) {
        fprintf(stderr, "appendToCSV: both Header and Values are NULL\n");
        return;
    }
    if (path == NULL) {
        fprintf(stderr, "appendToCSV: path is NULL\n");
        return;
    }

    if (h != NULL && v != NULL) {
        /* prefer using logger if available; fallback to stderr */
#ifdef LOG_ERROR
        LOG_ERROR("appendToCSV: export type must either be header or value, not both.\n");
#else
        fprintf(stderr, "appendToCSV: export type must either be header or value, not both.\n");
#endif
        return;
    }

    int fileEmpty = 1;
    FILE* fr = fopen(path, "rb");
    if (fr) {
        if (fseek(fr, 0, SEEK_END) == 0) {
            long sz = ftell(fr);
            if (sz > 0) fileEmpty = 0;
        }
        fclose(fr);
    }

    FILE* fw = fopen(path, "a");
    if (!fw) {
#ifdef LOG_ERROR
        LOG_ERROR("appendToCSV: failed to open %s for append\n", path);
#else
        fprintf(stderr, "appendToCSV: failed to open %s for append\n", path);
#endif
        return;
    }

    if (h != NULL && fileEmpty) {
        for (int i = 0; i < h->count; ++i) {
            if (i) fputc(',', fw);
            const char* col = (h->s && h->s[i]) ? h->s[i] : "";
            char* esc = escape_csv(col);
            if (esc) {
                fputs(esc, fw);
                free(esc);
            }
        }
        fputc('\n', fw);
    }

    if (v != NULL) {
        for (int i = 0; i < v->len; ++i) {
            if (i) fputc(',', fw);
            double val = v->value ? v->value[i] : 0.0;
            char* ds = format_double(val);
            if (ds) {
                fputs(ds, fw);
                free(ds);
            }
        }
        fputc('\n', fw);
    }

    fclose(fw);
}