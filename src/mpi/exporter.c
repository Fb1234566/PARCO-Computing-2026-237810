#include "exporter.h"
#include "logger.h"

static char* format_double(double d) {
    char buf[64];
    /* precisione fissa 6 decimali, si può adattare */
    snprintf(buf, sizeof(buf), "%.6f", d);
    /* trim trailing zeros */
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
	if (h!=nullptr && v!=nullptr) {
		LOG_ERROR("appendToCSV: export type must either be header or value, not  both.\n");
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
        LOG_ERROR(stderr, "appendToCSV: failed to open %s for append\n", path);
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